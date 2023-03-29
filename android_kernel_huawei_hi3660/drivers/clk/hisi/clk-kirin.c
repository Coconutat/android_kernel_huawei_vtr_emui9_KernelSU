/*
 * Hisilicon clock driver
 *
 * Copyright (c) 2013-2015 Hisilicon Limited.
 *
 *Author: zhaokai <zhaokai1@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/clk-private.h>
#endif
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/pm_opp.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include "hisi-clk-mailbox.h"
#include <linux/hwspinlock.h>
#include <soc_sctrl_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_pmctrl_interface.h>
#include "peri_dvfs/peri_volt_poll.h"
#include "clk-kirin.h"
#include <linux/mfd/hisi_pmic.h>

#ifndef CONFIG_HISI_CLK_ALWAYS_ON
/*lint -e750 -esym(750,*)*/
#define hi3xxx_CLK_GATE_DISABLE_OFFSET		0x4
#endif
#define hi3xxx_CLK_GATE_STATUS_OFFSET		0x8
/* reset register offset */
#define hi3xxx_RST_DISABLE_REG_OFFSET		0x4
#define CLK_GATE_ALWAYS_ON_MASK			0x4
#define WIDTH_TO_MASK(width)			((1 << (width)) - 1)
#define MAX_FREQ_NUM				8

#define HW_EN(n)				(0x10001 << (n))
#define HW_DIS(n)				(0x10000 << (n))

#define CLK_HWLOCK_TIMEOUT			1000
#define PPLLCTRL0(n)				(0x030 + ((n) << 3))
#define PPLLCTRL1(n)				(0x034 + ((n) << 3))
/*lint -e750 +esym(750,*)*/
#define PPLLCTRL0_EN			 	0
#define PPLLCTRL0_BP			 	1
#define PPLLCTRL0_LOCK		 		26
#define PPLLCTRL1_GT		 	 	26
#define ABB_SCBAKDATA(BASE)			SOC_SCTRL_SCBAKDATA12_ADDR(BASE);
#define PPLL3_SCBAKDATA(BASE)			SOC_SCTRL_SCBAKDATA11_ADDR(BASE);
#define AP_POLL_EN			 	0
#define LPM3_POLL_EN			 	1
#define AP_ABB_EN			 	0
#define LPM3_ABB_EN			 	1
#define AP_PPLL_STABLE_TIME			1000
#define AP_ABB_CODEC_LOCK			9
#define CLOCK_GATE_SYNC_MAX			10 /*us*/

#define PERI_AVS_LOOP_MAX			20
#define PERI_AVS_EDC0_SENSITIVE_FREQ		400000000
#define PERI_AVS_LDI0_SENSITIVE_FREQ		320000000 /*HZ*/

#define PPLL3_EN_ACPU				SOC_CRGPERIPH_PEREN0_ppll3_en_cpu_START
#define PPLL3_EN_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PEREN0_ADDR(ADDR)
#define PPLL3_DIS_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PERDIS0_ADDR(ADDR)
#define PPLL3_GT_ACPU				SOC_CRGPERIPH_PEREN11_ppll3_gt_cpu_START
#define PPLL3_GT_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PEREN11_ADDR(ADDR)
#define PPLL3_DISGT_ACPU(ADDR)			SOC_CRGPERIPH_PERDIS11_ADDR(ADDR)
#define PPLL2_EN_ACPU				SOC_CRGPERIPH_PEREN0_ppll2_en_cpu_START
#define PPLL2_EN_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PEREN0_ADDR(ADDR)
#define PPLL2_DIS_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PERDIS0_ADDR(ADDR)
#define PPLL2_GT_ACPU				SOC_CRGPERIPH_PEREN11_ppll2_gt_cpu_START
#define PPLL2_GT_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PEREN11_ADDR(ADDR)
#define PPLL2_DISGT_ACPU_ADDR(ADDR)		SOC_CRGPERIPH_PERDIS11_ADDR(ADDR)

#ifdef SOC_PMCTRL_PPLL7CTRL0_ppll7_en_START
#define PPLL7_EN						SOC_PMCTRL_PPLL7CTRL0_ppll7_en_START
#define PPLL7_EN_ACPU_ADDR(ADDR)		SOC_PMCTRL_PPLL7CTRL0_ADDR(ADDR)
#define PPLL7_GT						SOC_PMCTRL_PPLL7CTRL1_gt_clk_ppll7_START
#define PPLL7_GT_ACPU_ADDR(ADDR)		SOC_PMCTRL_PPLL7CTRL1_ADDR(ADDR)
#define PPLL7CTRL0(ADDR)				SOC_PMCTRL_PPLL7CTRL0_ADDR(ADDR)
#else
#define PPLL7_EN						0
#define PPLL7_EN_ACPU_ADDR(ADDR)		NULL
#define PPLL7_GT						0
#define PPLL7_GT_ACPU_ADDR(ADDR)		NULL
#define PPLL7CTRL0(ADDR)				NULL
#endif

#ifdef  SOC_PMCTRL_PPLL6CTRL0_ppll6_en_START
#define PPLL6_EN						SOC_PMCTRL_PPLL6CTRL0_ppll6_en_START
#define PPLL6_EN_ACPU_ADDR(ADDR)		SOC_PMCTRL_PPLL6CTRL0_ADDR(ADDR)
#define PPLL6_GT						SOC_PMCTRL_PPLL6CTRL1_gt_clk_ppll6_START
#define PPLL6_GT_ACPU_ADDR(ADDR)		SOC_PMCTRL_PPLL6CTRL1_ADDR(ADDR)
#define PPLL6CTRL0(ADDR)				SOC_PMCTRL_PPLL6CTRL0_ADDR(ADDR)
#else
#define PPLL6_EN						0
#define PPLL6_EN_ACPU_ADDR(ADDR)		NULL
#define PPLL6_GT						0
#define PPLL6_GT_ACPU_ADDR(ADDR)		NULL
#define PPLL6CTRL0(ADDR)				NULL
#endif

/*vivobus*/
#define SC_SEL_VIVOBUS_ADDR(BASE)		SOC_CRGPERIPH_CLKDIV10_ADDR(BASE)
#define SC_SEL_VIVOBUS_MASK			0x00003000
#define SC_SEL_VIVOBUS_SHIFT			SOC_CRGPERIPH_CLKDIV10_sc_sel_vivobus_START
#define GT_CLK_VIVOBUS_ADDR(BASE)		SOC_CRGPERIPH_CLKDIV20_ADDR(BASE)
#define GT_CLK_VIVOBUS				SOC_CRGPERIPH_CLKDIV20_gt_clk_vivobus_START
#define CLK_VIVOBUS_ADDR(BASE)			SOC_CRGPERIPH_PEREN3_ADDR(BASE)
#define CLK_VIVOBUS				SOC_CRGPERIPH_PEREN3_gt_clk_vivobus_START
/*
 * The reverse of DIV_ROUND_UP: The maximum number which
 * divided by m is r
 */
#define MULT_ROUND_UP(r, m)			((r) * (m) + (m) - 1)
#define INVALID_HWSPINLOCK_ID			0xFF

#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
#define CLK_DVFS_IPC_CMD			0xC
static u32 g_count_num_dvfs = 0;
#endif

enum {
	PPLL0 = 0,
	PPLL1,
	PPLL2,
	PPLL3,
	PPLL6 = 0x6,
	PPLL7 = 0x7,
	PPLLMAX,
};

struct hi3xxx_periclk {
	struct clk_hw	hw;
	void __iomem	*enable;	/* enable register */
	void __iomem	*reset;		/* reset register */
	u32		ebits;		/* bits in enable/disable register */
	u32		rbits;		/* bits in reset/unreset register */
	void __iomem	*sctrl;		/*sysctrl addr*/
	void __iomem	*pmctrl;	/*pmctrl addr*/
	const char 	*friend;
	spinlock_t	*lock;
	u32		flags;
	struct hwspinlock	*clk_hwlock;
	u32		peri_dvfs_sensitive;/*0:non,1:direct avs,rate(HZ):sensitive rate*/
	u32		perivolt_poll_id;
	u32		sensitive_pll;
	u32		always_on;
	u32		gate_abandon_enable;
	u32		sync_time;
	u32		clock_id;
	int		pmu_clk_enable;
};

struct hi3xxx_muxclk {
	struct clk_hw	hw;
	void __iomem	*reg;		/* mux register */
	u8		shift;
	u8		width;
	u32		mbits;		/* mask bits in mux register */
	spinlock_t	*lock;
};

struct hi3xxx_divclk {
	struct clk_hw	hw;
	void __iomem	*reg;		/* divider register */
	u8		shift;
	u8		width;
	u32		mbits;		/* mask bits in divider register */
	const struct clk_div_table	*table;
	spinlock_t	*lock;
};

/* ppll */
struct hi3xxx_ppll_clk {
	struct clk_hw	hw;
	u32		ref_cnt;	/* reference count */
	u32		en_cmd[LPM3_CMD_LEN];
	u32		dis_cmd[LPM3_CMD_LEN];
	void __iomem	*addr;		/*base addr*/
	void __iomem	*endisable_addr;
	void __iomem	*sctrl;		/*sysctrl addr*/
	u32		flags;
	u32		clock_id;
	struct hwspinlock	*clk_hwlock;
	spinlock_t		*lock;
};

struct hi3xxx_xfreq_clk {
	struct clk_hw	hw;
	void __iomem	*reg;		/* ctrl register */

	u32		id;
	u32		set_rate_cmd[LPM3_CMD_LEN];
	u32		get_rate_cmd[LPM3_CMD_LEN];
	u32		freq[MAX_FREQ_NUM];
	u32		volt[MAX_FREQ_NUM];
	u32		table_length;

	u32		rate;
};

struct hi3xxx_xfreq_pll {
	struct clk_hw	hw;
	void __iomem	*reg;		/* pll ctrl0 register */
};

struct hi3xxx_mclk {
	struct clk_hw	hw;
	u32		ref_cnt;	/* reference count */
	u32		en_cmd[LPM3_CMD_LEN];
	u32		dis_cmd[LPM3_CMD_LEN];
	u32		clock_id;
	u32		always_on;
	u32		gate_abandon_enable;
	spinlock_t	*lock;
};

struct hs_clk {
	void __iomem	*pmctrl;
	void __iomem	*sctrl;
	void __iomem	*crgctrl;
	void __iomem	*pmuctrl;
	void __iomem	*pctrl;
	void __iomem	*mediacrg;
	void __iomem	*iomcucrg;
	void __iomem	*media1crg;
	void __iomem	*media2crg;
	spinlock_t	lock;
};
static struct hwspinlock	*clk_hwlock_9;
static void __iomem __init *hs_clk_get_base(struct device_node *np);

static struct hs_clk hs_clk = {
	.lock = __SPIN_LOCK_UNLOCKED(hs_clk.lock),
};

#ifdef CONFIG_HISI_CLK
int IS_FPGA(void)
{
	static int flag_diff_fpga_asic = -1;
	if (flag_diff_fpga_asic == -1) {
		if (of_find_node_by_name(NULL, "fpga")) {
			flag_diff_fpga_asic = 1;
		} else {
			flag_diff_fpga_asic = 0;
		}
	}
	return flag_diff_fpga_asic;
}
EXPORT_SYMBOL_GPL(IS_FPGA);
#endif

extern int __clk_enable(struct clk *clk);
extern void __clk_disable(struct clk *clk);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
extern int __clk_prepare(struct clk *clk);
extern void __clk_unprepare(struct clk *clk);
#else
extern int clk_core_prepare(struct clk_core *clk_core);
extern void clk_core_unprepare(struct clk_core *clk_core);
#endif

#ifdef CONFIG_HISI_PERIDVFS
static int peri_dvfs_set_volt(u32 peri_dvfs_sensitive, u32 poll_id, u32 volt_level)
{
	struct peri_volt_poll *pvp = NULL;
	int ret = 0;
	int volt = 0;
	int loop = PERI_AVS_LOOP_MAX;

	if (!peri_dvfs_sensitive)
		return 0;
	pvp = peri_volt_poll_get(poll_id, NULL);
	if (!pvp) {
		pr_err("pvp get failed!\n");
		return -EINVAL;
	}
	ret = peri_set_volt(pvp, volt_level);
	if (ret < 0) {
		pr_err("[%s]set volt failed ret=%d!\n", __func__, ret);
		return ret;
	}
	if (volt_level == PERI_VOLT_2) {
		do {
			volt = peri_get_volt(pvp);
			if (volt != PERI_VOLT_2) {
				loop--;
				usleep_range(1500, 3000);
			}
		} while (volt != PERI_VOLT_2 && loop > 0);
		if (volt != PERI_VOLT_2) {
			pr_err("[%s] fail to updata volt, ret = %d!\n",
				__func__, volt);
		}
	}
	return ret;
}
#endif

static int hi3xxx_clkgate_prepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk = NULL;
	struct clk *friend_clk;
	int ret = 0;
#ifdef CONFIG_HISI_PERIDVFS
	unsigned long cur_rate = 0;
#endif

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	/*if friend clk exist,enable it*/
	if (pclk->friend) {
		friend_clk = __clk_lookup(pclk->friend);
		if (IS_ERR_OR_NULL(friend_clk)) {
			pr_err("%s get failed!\n", pclk->friend);
			return -1;
		}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
		ret = __clk_prepare(friend_clk);
#else
		ret = clk_core_prepare(friend_clk->core);
#endif
		if (ret) {
			pr_err("[%s], friend clock prepare faild!", __func__);
			return ret;
		}
	}
#ifdef CONFIG_HISI_PERIDVFS
	/*only ldi0 according to freq sensitive avs
	 peri_dvfs_sensitive
		0: non
		1(): direct avs
		rate(HZ): if is according sensitive rate
	*/
	if (!strcmp(__clk_get_name(hw->clk), "clk_ldi0")) {/*lint !e421*/
		cur_rate = clk_get_rate(hw->clk);
		if (!cur_rate)
			pr_err("[%s]soft rate:%ld must not be 0,please check!\n", __func__, cur_rate);
		if (PERI_AVS_LDI0_SENSITIVE_FREQ <= cur_rate)
			ret = peri_dvfs_set_volt(pclk->peri_dvfs_sensitive, pclk->perivolt_poll_id, PERI_VOLT_2);
	} else if (!strcmp(__clk_get_name(hw->clk), "clk_edc0")) {/*lint !e421*/
		cur_rate = clk_get_rate(hw->clk);
		if (!cur_rate)
			pr_err("[%s]soft rate:%ld must not be 0,please check!\n", __func__, cur_rate);
		if (PERI_AVS_EDC0_SENSITIVE_FREQ < cur_rate)
			ret = peri_dvfs_set_volt(pclk->peri_dvfs_sensitive, pclk->perivolt_poll_id, PERI_VOLT_2);
	} else {
		ret = peri_dvfs_set_volt(pclk->peri_dvfs_sensitive, pclk->perivolt_poll_id, PERI_VOLT_2);
	}
#endif
	return ret;
}

static int hi3xxx_clkgate_enable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	struct clk *friend_clk;
	int ret = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	/*gate sync*/
	if (pclk->sync_time > 0)
		udelay(pclk->sync_time);

	/*sft give up enable*/
	if (pclk->gate_abandon_enable)
		return 0;

	/*enable clock*/
	if (pclk->enable)
		writel(pclk->ebits, pclk->enable);

	/* disable reset register */
	if (pclk->reset)
		writel(pclk->rbits, pclk->reset + hi3xxx_RST_DISABLE_REG_OFFSET);

	/*if friend clk exist,enable it*/
	if (pclk->friend) {
		friend_clk = __clk_lookup(pclk->friend);
		if (IS_ERR_OR_NULL(friend_clk)) {
			pr_err("%s get failed!\n", pclk->friend);
			return -1;
		}
		ret = __clk_enable(friend_clk);
		if (ret) {
			pr_err("[%s], friend clock:%s enable faild!", __func__, pclk->friend);
			return ret;
		}
	}

	if (pclk->sync_time > 0)
		udelay(pclk->sync_time);

	return 0;
}

static void hi3xxx_clkgate_disable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	struct clk *friend_clk;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	/* reset the ip, then disalbe clk */
	if (pclk->reset)
		writel(pclk->rbits, pclk->reset);

#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	if (pclk->enable) {
		if (!pclk->always_on)
			writel(pclk->ebits, pclk->enable + hi3xxx_CLK_GATE_DISABLE_OFFSET);
	}
	if (pclk->sync_time > 0)
		udelay(pclk->sync_time);
	/*if friend clk exist, disable it .*/
	if (pclk->friend) {
		friend_clk = __clk_lookup(pclk->friend);
		if (IS_ERR_OR_NULL(friend_clk)) {
			pr_err("%s get failed!\n", pclk->friend);
		}
		__clk_disable(friend_clk);
	}
#endif
}

static void hi3xxx_clkgate_unprepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk = NULL;
	struct clk *friend_clk;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

#ifdef CONFIG_HISI_PERIDVFS
	peri_dvfs_set_volt(pclk->peri_dvfs_sensitive, pclk->perivolt_poll_id, PERI_VOLT_0);
#endif
#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	if (pclk->friend) {
		friend_clk = __clk_lookup(pclk->friend);
		if (IS_ERR_OR_NULL(friend_clk)) {
			pr_err("%s get failed!\n", pclk->friend);
            return;
		}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
		__clk_unprepare(friend_clk);
#else
		clk_core_unprepare(friend_clk->core);
#endif
	}
#endif
}

#ifdef CONFIG_HISI_CLK_DEBUG
static int hi3xxx_clkgate_is_enabled(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	u32 reg = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	if (pclk->enable) {
#ifdef CONFIG_HISI_HI6250_CLK
		if ((!strcmp(__clk_get_name(hw->clk), "clk_dss_axi_mm"))
				|| (!strcmp(__clk_get_name(hw->clk), "pclk_mmbuf"))
				|| (!strcmp(__clk_get_name(hw->clk), "aclk_mmbuf")))
			reg = readl(pclk->enable + 0x18);
		else{
			reg = readl(pclk->enable + hi3xxx_CLK_GATE_STATUS_OFFSET);
		}
#else
		reg = readl(pclk->enable + hi3xxx_CLK_GATE_STATUS_OFFSET);
#endif
	} else
		return 2;

	reg &= pclk->ebits;

	return reg ? 1 : 0;
}

static void __iomem *hi3xxx_clkgate_get_reg(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	void __iomem	*ret = NULL;
	u32 val = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	if (pclk->enable) {
		ret = pclk->enable + hi3xxx_CLK_GATE_STATUS_OFFSET;
		val = readl(ret);
		val &= pclk->ebits;
		pr_info("\n[%s]: reg = 0x%pK, bits = 0x%x, regval = 0x%x\n",
			__clk_get_name(hw->clk), ret, pclk->ebits, val);
	}

	return ret;
}

static int hi3xxx_dumpgate(struct clk_hw *hw, char* buf)
{
	struct hi3xxx_periclk *pclk;
	void __iomem	*ret = NULL;
	u32 val = 0;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	if (pclk->enable && buf) {
		ret = pclk->enable + hi3xxx_CLK_GATE_STATUS_OFFSET;
		val = readl(ret);
		snprintf(buf, DUMP_CLKBUFF_MAX_SIZE, "[%s] : regAddress = 0x%pK, regval = 0x%x\n", __clk_get_name(hw->clk), ret, val);
	}
	return 0;
}
#endif

#ifdef CONFIG_HISI_CLK
static int clk_gate_get_source(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	return pclk->sensitive_pll;
}
#endif

static struct clk_ops hi3xxx_clkgate_ops = {
	.prepare        = hi3xxx_clkgate_prepare,
	.unprepare      = hi3xxx_clkgate_unprepare,
	.enable		= hi3xxx_clkgate_enable,
	.disable	= hi3xxx_clkgate_disable,
#ifdef CONFIG_HISI_CLK
	.get_source = clk_gate_get_source,
#endif
#ifdef CONFIG_HISI_CLK_DEBUG
	.is_enabled = hi3xxx_clkgate_is_enabled,
	.get_reg  = hi3xxx_clkgate_get_reg,
	.dump_reg = hi3xxx_dumpgate,
#endif
};

static void __init hi3xxx_clkgate_setup(struct device_node *np)
{
	struct hi3xxx_periclk *pclk;
	struct clk_init_data *init;
	struct clk *clk;
	const char *clk_name, *name, *clk_friend, *parent_names;
	void __iomem *reg_base;
	u32 rdata[2] = {0};
	u32 gdata[2] = {0};
	u32 sync_time = 0;
	u32 clock_id = 0;
	u32 lock_id = 0;
	u32 sensitive_pll = 0;

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] %s node doesn't have clock-output-name property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,hi3xxx-clkgate",
				       &gdata[0], 2)) {
		pr_err("[%s] %s node doesn't have hi3xxx-clkgate property!\n",
			 __func__, np->name);
		return;
	}

	if (of_property_read_string(np, "clock-friend-names", &clk_friend))
		clk_friend = NULL;
	if (of_property_read_bool(np, "clock-id")) {
		if (of_property_read_u32_array(np, "clock-id", &clock_id, 1)) {
			pr_err("[%s] %s clock-id property is null\n",
				 __func__, np->name);
		}
	}
	if (of_property_read_bool(np, "sensitive_pll")) {
		if (of_property_read_u32(np, "sensitive_pll", &sensitive_pll)) {
			pr_err("[%s] %s node doesn't have sensitive_pll property!\n",
				 __func__, np->name);
			return;
		}
	}
	if (NULL != of_find_property(np, "hwspinlock-id", NULL)) {
		if (of_property_read_u32_array(np, "hwspinlock-id", &lock_id, 1)) {
			pr_err("[%s] %s node doesn't have hwspinliock-id property!\n",
				__func__, np->name);
			return;
		}
	}

	if (IS_FPGA()) {
		if (NULL != of_find_property(np, "clock-fpga", NULL)) {
			if (of_property_read_string(np, "clock-fpga", &parent_names)) {
				pr_err("[%s] %s node clock-fpga value is NULL!\n",
					__func__, np->name);
				return;
			}
		} else {
			 parent_names = of_clk_get_parent_name(np, 0);
		}
	} else {
		parent_names = of_clk_get_parent_name(np, 0);
	}

	pclk = kzalloc(sizeof(struct hi3xxx_periclk), GFP_KERNEL);
	if (!pclk) {
		pr_err("[%s] fail to alloc pclk!\n", __func__);
		return;
	}

	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);
	init->ops = &hi3xxx_clkgate_ops;
	init->flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED;
	init->parent_names = &parent_names;
	init->num_parents = 1;

	if (of_property_read_u32_array(np, "hisilicon,hi3xxx-clkreset",
				       &rdata[0], 2)) {
		pclk->reset = NULL;
		pclk->rbits = 0;
	} else {
		pclk->reset = reg_base + rdata[0];
		pclk->rbits = rdata[1];
	}

	pclk->peri_dvfs_sensitive = 0;
	if (of_property_read_bool(np, "peri_dvfs_sensitive")) {
		pclk->peri_dvfs_sensitive = 1;
	}

	if (of_property_read_bool(np, "always_on"))
		pclk->always_on = 1;
	else
		pclk->always_on = 0;
	if (of_property_read_bool(np, "gate_abandon_enable"))
		pclk->gate_abandon_enable = 1;
	else
		pclk->gate_abandon_enable = 0;
	if (of_property_read_u32_array(np, "gate_sync_time",
				       &sync_time, 1)) {
		pclk->sync_time = 0;
	} else {
		WARN_ON(sync_time > CLOCK_GATE_SYNC_MAX);
		pclk->sync_time = sync_time;
	}

	/* if gdata[1] is 0, represents the enable reg is fake */
	if (gdata[1] == 0)
			pclk->enable = NULL;
	else
			pclk->enable = reg_base + gdata[0];
	
    pclk->ebits = gdata[1];
	pclk->lock = &hs_clk.lock;
	pclk->hw.init = init;
	pclk->friend = clk_friend;
	pclk->flags = lock_id;
	pclk->clock_id = clock_id;
	pclk->perivolt_poll_id = clock_id;
	pclk->sensitive_pll = sensitive_pll;
	pclk->clk_hwlock = NULL;
	pclk->sctrl = NULL;
	pclk->pmctrl = hs_clk_base(HS_PMCTRL);
	pclk->pmu_clk_enable = 0;

	clk = clk_register(NULL, &pclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to reigister clk %s!\n",
			__func__, clk_name);
		goto err_clk;
	}
	if (!of_property_read_string(np, "clock-output-names", &name))
		clk_register_clkdev(clk, name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;/*lint !e429*/
err_clk:
	kfree(init);
	init = NULL;
err_init:
	kfree(pclk);
	pclk = NULL;
	return;
}
#ifdef CONFIG_HISI_HI6421V600_PMU
static int hi3xxx_multicore_clkgate_prepare(struct clk_hw *hw)
{
	return 0;
}

static int hi3xxx_multicore_clkgate_enable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	int val = 0;
	unsigned long flags;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	if (pclk->gate_abandon_enable)
		return 0;
	flags = 0;
	if (pclk->lock)
		spin_lock_irqsave(pclk->lock, flags);

	val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
	val |= pclk->ebits;
	hisi_pmic_reg_write(pclk->pmu_clk_enable, val);

	if (pclk->lock)
		spin_unlock_irqrestore(pclk->lock, flags);
	return 0;
}

static void hi3xxx_multicore_clkgate_disable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	int val = 0;
	unsigned long flags;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	flags = 0;
	if (pclk->lock)
		spin_lock_irqsave(pclk->lock, flags);

	if (!pclk->always_on) {
		val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
		val &= (~pclk->ebits);
		hisi_pmic_reg_write(pclk->pmu_clk_enable, val);
	}

	if (pclk->lock)
		spin_unlock_irqrestore(pclk->lock, flags);

	return;
#endif
}

static void hi3xxx_multicore_clkgate_unprepare(struct clk_hw *hw)
{
	return;
}

#else
static int hi3xxx_multicore_clkgate_prepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);
	if (AP_ABB_CODEC_LOCK == pclk->flags) {
		if (NULL == clk_hwlock_9) {
			clk_hwlock_9 = hwspin_lock_request_specific(pclk->flags);
			if (NULL == clk_hwlock_9) {
				pr_err("pmu clk request hwspin lock failed !\n");
				return -ENODEV;
			}
		}
		pclk->clk_hwlock = clk_hwlock_9;
	} else {
		pclk->clk_hwlock = hwspin_lock_request_specific(pclk->flags);
		if (NULL == pclk->clk_hwlock) {
			pr_err("multicore clk request hwspin lock failed !\n");
			return -ENODEV;
		}
	}

	return 0;
}

static int hi3xxx_multicore_clkgate_enable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	int val = 0;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

	if (pclk->gate_abandon_enable)
		return 0;
	if (hwspin_lock_timeout(pclk->clk_hwlock, CLK_HWLOCK_TIMEOUT)) {
		pr_err("multicore enable hwspinlock timout!\n");
		return -ENOENT;
	}

	if (pclk->pmu_clk_enable) {
		val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
		val |= pclk->ebits;
		hisi_pmic_reg_write(pclk->pmu_clk_enable, val);
	}
	hwspin_unlock(pclk->clk_hwlock);
	return 0;
}

static void hi3xxx_multicore_clkgate_disable(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	int val = 0;
	pclk = container_of(hw, struct hi3xxx_periclk, hw);

#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	if (hwspin_lock_timeout(pclk->clk_hwlock, CLK_HWLOCK_TIMEOUT)) {
		pr_err("multicore disable hwspinlock timout!\n");
		return;
	}

	if (pclk->pmu_clk_enable) {
		if (!pclk->always_on) {
			val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
			val &= (~pclk->ebits);
			hisi_pmic_reg_write(pclk->pmu_clk_enable, val);
		}
	}
	hwspin_unlock(pclk->clk_hwlock);
	return;
#endif
}

static void hi3xxx_multicore_clkgate_unprepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);
	if (AP_ABB_CODEC_LOCK != pclk->flags) {
		if (hwspin_lock_free(pclk->clk_hwlock)) {
			pr_err("multicore hwspinlock free %d failed!\n", hwspin_lock_get_id(pclk->clk_hwlock));
			return;
		}
		pclk->clk_hwlock = NULL;
	}
	return;
}
#endif

static struct clk_ops hi3xxx_pmu_clkgate_ops = {
	.prepare        = hi3xxx_multicore_clkgate_prepare,
	.unprepare      = hi3xxx_multicore_clkgate_unprepare,
	.enable		= hi3xxx_multicore_clkgate_enable,
	.disable        = hi3xxx_multicore_clkgate_disable,
};

static int hi3xxx_multicore_abb_clkgate_prepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	u32 val = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);
	if (NULL == clk_hwlock_9) {
		clk_hwlock_9 = hwspin_lock_request_specific(pclk->flags);
		if (NULL == clk_hwlock_9) {
			pr_err("abb clk request hwspin lock failed !\n");
			return -ENODEV;
		}


	}
	pclk->clk_hwlock = clk_hwlock_9;
	if (hwspin_lock_timeout(pclk->clk_hwlock, CLK_HWLOCK_TIMEOUT)) {
		pr_err("abb clk enable hwspinlock timout!\n");
		return -ENOENT;
	}

	val = readl(pclk->sctrl);
	if ((0 == (val & BIT(AP_ABB_EN)))) {
		if (0 == (val & BIT(LPM3_ABB_EN))) {
			/* open abb clk */
			val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
			val |= pclk->ebits;
			hisi_pmic_reg_write(pclk->pmu_clk_enable, val);
		}
		/* write 0x344 register */
		val = readl(pclk->sctrl);
		val |= BIT(AP_ABB_EN);
		writel(val, pclk->sctrl);
	}

	hwspin_unlock(pclk->clk_hwlock);
	mdelay(1);
	return 0;
}

static void hi3xxx_multicore_abb_clkgate_unprepare(struct clk_hw *hw)
{
	struct hi3xxx_periclk *pclk;
	u32 val = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);

#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	if (hwspin_lock_timeout(pclk->clk_hwlock, CLK_HWLOCK_TIMEOUT)) {
		pr_err("abb clk disable hwspinlock timout!\n");
		return;
	}

	val = readl(pclk->sctrl);
	if (1 == (val & BIT(AP_ABB_EN))) {
		if (0 == (val & BIT(LPM3_ABB_EN))) {
			if (!pclk->always_on) {
				/* close abb clk */
				val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
				val &= (~pclk->ebits);
				hisi_pmic_reg_write(pclk->pmu_clk_enable, val);
			}
		}
		/* write 0x344 register */
		val = readl(pclk->sctrl);
		val &= (~BIT(AP_ABB_EN));
		writel(val, pclk->sctrl);
	}
	hwspin_unlock(pclk->clk_hwlock);
#endif
	return;
}

#ifdef CONFIG_HISI_CLK_DEBUG
static int hi3xxx_dump_abbclk(struct clk_hw *hw, char* buf)
{
	struct hi3xxx_periclk *pclk;
	u32 val = 0;

	pclk = container_of(hw, struct hi3xxx_periclk, hw);
	if (NULL == clk_hwlock_9) {
		clk_hwlock_9 = hwspin_lock_request_specific(pclk->flags);
		if (NULL == clk_hwlock_9) {
			pr_err("abb clk request hwspin lock failed !\n");
			return -ENODEV;
		}


	}
	pclk->clk_hwlock = clk_hwlock_9;
	if (hwspin_lock_timeout(pclk->clk_hwlock, CLK_HWLOCK_TIMEOUT)) {
		pr_err("abb clk enable hwspinlock timout!\n");
		return -ENOENT;
	}
	if(buf){
		val = hisi_pmic_reg_read(pclk->pmu_clk_enable);
		snprintf(buf, DUMP_CLKBUFF_MAX_SIZE, "[%s] : regAddress = 0x%x, regval = 0x%x\n", __clk_get_name(hw->clk), pclk->pmu_clk_enable, val);
	}
	hwspin_unlock(pclk->clk_hwlock);
	return 0;

}
#endif

static struct clk_ops hi3xxx_abb_clkgate_ops = {
	.prepare        = hi3xxx_multicore_abb_clkgate_prepare,
	.unprepare      = hi3xxx_multicore_abb_clkgate_unprepare,
#ifdef CONFIG_HISI_CLK_DEBUG
	.dump_reg = hi3xxx_dump_abbclk,
#endif
};
static void __init hi3xxx_pmu_clkgate_setup(struct device_node *np)
{
	struct hi3xxx_periclk *pclk;
	struct clk_init_data *init;
	struct clk *clk;
	const char *clk_name, *name, *clk_friend, *parent_names;
	void __iomem *reg_base;
	u32 rdata[2] = {0};
	u32 gdata[2] = {0};
	u32 lock_id = 0;
	u32 clock_id = 0;
	struct device_node *sctrl_np;

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] %s node doesn't have clock-output-name property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,clkgate", &gdata[0], 2)) {
		pr_err("[%s] %s node doesn't have hisilicon,clkgate property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hwspinlock-id", &lock_id, 1)) {
		pr_info("[%s] %s node doesn't have hwspinliock-id property!\n",
			 __func__, np->name);
		lock_id = INVALID_HWSPINLOCK_ID;
	}
	if (of_property_read_bool(np, "clock-id")) {
		if (of_property_read_u32_array(np, "clock-id", &clock_id, 1)) {
			pr_err("[%s] %s clock_id property is null!\n",
				 __func__, np->name);
		}
	}

	if (of_property_read_string(np, "clock-friend-names", &clk_friend))
		clk_friend = NULL;

	if (IS_FPGA()) {
		if (NULL != of_find_property(np, "clock-fpga", NULL)) {
			if (of_property_read_string(np, "clock-fpga", &parent_names)) {
				pr_err("[%s] %s node clock-fpga value is NULL!\n",
					__func__, np->name);
				return;
			}
		} else {
			 parent_names = of_clk_get_parent_name(np, 0);
		}
	} else {
		parent_names = of_clk_get_parent_name(np, 0);
	}

	pclk = kzalloc(sizeof(struct hi3xxx_periclk), GFP_KERNEL);
	if (!pclk) {
		pr_err("[%s] fail to alloc pclk!\n", __func__);
		return;
	}

	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);

	if (!strcmp(clk_name, "clk_abb_192")) {/*lint !e421*/
		init->ops = &hi3xxx_abb_clkgate_ops;
	} else {
		init->ops = &hi3xxx_pmu_clkgate_ops;
	}
	init->flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED;
	init->parent_names = &parent_names;
	init->num_parents = 1;

	if (of_property_read_u32_array(np, "hisilicon,hi3xxx-clkreset",
				       &rdata[0], 2)) {
		pclk->reset = NULL;
		pclk->rbits = 0;
	} else {
		pclk->reset = reg_base + rdata[0];
		pclk->rbits = rdata[1];
	}

	if (of_property_read_bool(np, "always_on"))
		pclk->always_on = 1;
	else
		pclk->always_on = 0;

	if (of_property_read_bool(np, "gate_abandon_enable"))
		pclk->gate_abandon_enable = 1;
	else
		pclk->gate_abandon_enable = 0;

	sctrl_np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (NULL == sctrl_np) {
		pr_err("[%s] fail to find sctrl node!\n", __func__);
		goto err_sctr;
	}
	pclk->sctrl = of_iomap(sctrl_np, 0);
	if (!pclk->sctrl) {
		pr_err("[%s]failed to iomap!\n", __func__);
		goto no_iomap;
	}
	pclk->sctrl = ABB_SCBAKDATA(pclk->sctrl);/*only for abb clk*/
	pclk->enable = NULL;
	pclk->ebits = BIT(gdata[1]);
	pclk->lock = &hs_clk.lock;
	pclk->hw.init = init;
	pclk->friend = clk_friend;
	pclk->flags = lock_id;
	pclk->clock_id = clock_id;
	pclk->clk_hwlock = NULL;
	pclk->pmu_clk_enable = gdata[0];

	clk = clk_register(NULL, &pclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to reigister clk %s!\n",
			__func__, clk_name);
		goto err_clk;
	}
	if (!of_property_read_string(np, "clock-output-names", &name))
		clk_register_clkdev(clk, name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;/*lint !e429*/

err_clk:
	iounmap(pclk->sctrl);
	pclk->sctrl = NULL;
no_iomap:
	of_node_put(sctrl_np);
err_sctr:
	kfree(init);
	init = NULL;
err_init:
	kfree(pclk);
	pclk = NULL;
	return;
}

static int ppll_enable_open(struct hi3xxx_ppll_clk *ppll_clk, int ppll)
{
	u32 val;
	/*en*/
	switch (ppll) {
	case PPLL3:
		val = readl(PPLL3_EN_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL3_EN_ACPU);
		writel(val, PPLL3_EN_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL2:
		val = readl(PPLL2_EN_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL2_EN_ACPU);
		writel(val, PPLL2_EN_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL6:
		val = readl(PPLL6_EN_ACPU_ADDR(ppll_clk->addr));//lint !e732
		val |= BIT(PPLL6_EN);
		writel(val, PPLL6_EN_ACPU_ADDR(ppll_clk->addr));
		break;
 	case PPLL7:
		val = readl(PPLL7_EN_ACPU_ADDR(ppll_clk->addr));
		val |= BIT(PPLL7_EN);
		writel(val, PPLL7_EN_ACPU_ADDR(ppll_clk->addr));
		break;
	default:
		pr_err("[%s]: A wrong PPLL-%d is enable_open\n", __func__, ppll);
		break;
	}
	return 0;
}

static void ppll_nogate(struct hi3xxx_ppll_clk *ppll_clk, int ppll)
{
	u32 val;

	/*output clock not gate*/
	switch (ppll) {
	case PPLL3:
		val = readl(PPLL3_GT_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL3_GT_ACPU);
		writel(val, PPLL3_GT_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL2:
		val = readl(PPLL2_GT_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL2_GT_ACPU);
		writel(val, PPLL2_GT_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL6:
		val = (unsigned int)readl(PPLL6_GT_ACPU_ADDR(ppll_clk->addr));
		val |= BIT(PPLL6_GT);
		writel(val, PPLL6_GT_ACPU_ADDR(ppll_clk->addr));
		break;
	case PPLL7:
		val = (unsigned int)readl(PPLL7_GT_ACPU_ADDR(ppll_clk->addr));
		val |= BIT(PPLL7_GT);
		writel(val, PPLL7_GT_ACPU_ADDR(ppll_clk->addr));
		break;
	default:
		pr_err("[%s]: A wrong PPLL-%d is enable_ready\n", __func__, ppll);
		break;
	}
}

static int ppll_enable_ready(struct hi3xxx_ppll_clk *ppll_clk, int ppll)
{
	u32 val;
	u32 timeout;
	timeout = 0;
	/*waiting lock*/

	switch (ppll) {
	case PPLL2:
	case PPLL3:
		do {
			val = readl(ppll_clk->addr + PPLLCTRL0(ppll));//lint !e732
			val &= BIT(PPLLCTRL0_LOCK);
			timeout++;
			if (AP_PPLL_STABLE_TIME < timeout)
				pr_err("%s: ppll-%d enable is timeout\n", __func__,ppll);
		} while (!val);
		break;
	case PPLL6:
		do {
			val = readl(PPLL6CTRL0(ppll_clk->addr));//lint !e732
			val &= BIT(PPLLCTRL0_LOCK);
			timeout++;
			if(AP_PPLL_STABLE_TIME < timeout)
				pr_err("%s: ppll-%d enable is timeout\n", __func__,ppll);
		} while (!val);
		break;
	case PPLL7:
		do {
			val = readl(PPLL7CTRL0(ppll_clk->addr));//lint !e732
			val &= BIT(PPLLCTRL0_LOCK);
			timeout++;
			if(AP_PPLL_STABLE_TIME < timeout)
				pr_err("%s: ppll-%d enable is timeout\n", __func__,ppll);
		} while (!val);
		break;
	default:
		pr_err("[%s]: A wrong PPLL-%d is enable_ready\n", __func__, ppll);
		break;
	}
	ppll_nogate(ppll_clk, ppll);
	return 0;
}

static void ppll_disable(struct hi3xxx_ppll_clk *ppll_clk, int ppll)
{
	u32 val;
	/*output clock gate*/
	switch (ppll) {
	case PPLL3:
		/*output clock gate*/
		val = readl(PPLL3_DISGT_ACPU(ppll_clk->endisable_addr));
		val |= BIT(PPLL3_GT_ACPU);
		writel(val, PPLL3_DISGT_ACPU(ppll_clk->endisable_addr));
		/*~en*/
		val = readl(PPLL3_DIS_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL3_EN_ACPU);
		writel(val, PPLL3_DIS_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL2:
		/*output gate*/
		val = readl(PPLL2_DISGT_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL2_GT_ACPU);
		writel(val, PPLL2_DISGT_ACPU_ADDR(ppll_clk->endisable_addr));
		/*~en*/
		val = readl(PPLL2_DIS_ACPU_ADDR(ppll_clk->endisable_addr));
		val |= BIT(PPLL2_EN_ACPU);
		writel(val, PPLL2_DIS_ACPU_ADDR(ppll_clk->endisable_addr));
		break;
	case PPLL6:
		/*output gate*/
		val = (unsigned int)readl(PPLL6_GT_ACPU_ADDR(ppll_clk->addr));
		val &= (~ BIT(PPLL6_GT));
		writel(val, PPLL6_GT_ACPU_ADDR(ppll_clk->addr));
		/*~en*/
		val = (unsigned int)readl(PPLL6_EN_ACPU_ADDR(ppll_clk->addr));//lint !e838
		val &= (~ BIT(PPLL6_EN));
		writel(val, PPLL6_EN_ACPU_ADDR(ppll_clk->addr));
		break;
	case PPLL7:
		/*output gate*/
		val = (unsigned int)readl(PPLL7_GT_ACPU_ADDR(ppll_clk->addr));
		val &= (~ BIT(PPLL7_GT));
		writel(val, PPLL7_GT_ACPU_ADDR(ppll_clk->addr));
		/*~en*/
		val = (unsigned int)readl(PPLL7_EN_ACPU_ADDR(ppll_clk->addr));
		val &= (~ BIT(PPLL7_EN));
		writel(val, PPLL7_EN_ACPU_ADDR(ppll_clk->addr));
		break;
	default:
		pr_err("[%s]: A wrong PPLL-%d is disable\n", __func__, ppll);
		break;
	}
}

static int hi3xxx_multicore_ppll_enable(struct clk_hw *hw)
{
	struct hi3xxx_ppll_clk *ppll_clk;

	ppll_clk = container_of(hw, struct hi3xxx_ppll_clk, hw);

	/*for debug*/
	ppll_clk->ref_cnt++;

	if (PPLL0 == ppll_clk->en_cmd[1])
		return 0;
	if (1 == ppll_clk->ref_cnt) {
		ppll_enable_open(ppll_clk, ppll_clk->en_cmd[1]);
		ppll_enable_ready(ppll_clk, ppll_clk->en_cmd[1]);
	}
	return 0;
}

static void hi3xxx_multicore_ppll_disable(struct clk_hw *hw)
{
	struct hi3xxx_ppll_clk *ppll_clk;

	ppll_clk = container_of(hw, struct hi3xxx_ppll_clk, hw);

	/*for debug*/
	ppll_clk->ref_cnt--;

	if (PPLL0 == ppll_clk->dis_cmd[1])
		return ;
#ifndef CONFIG_HISI_CLK_ALWAYS_ON
	if (!ppll_clk->ref_cnt)
		ppll_disable(ppll_clk, ppll_clk->dis_cmd[1]);
#endif
}

static struct clk_ops hi3xxx_ppll_ops = {
	.enable		= hi3xxx_multicore_ppll_enable,
	.disable		= hi3xxx_multicore_ppll_disable,
};

static void __init hi3xxx_ppll_setup(struct device_node *np)
{
	struct hi3xxx_ppll_clk *ppll_clk;
	struct clk_init_data *init;
	struct clk *clk;
	const char *clk_name, *parent_names;
	u32 en_cmd[LPM3_CMD_LEN] = {0};
	u32 dis_cmd[LPM3_CMD_LEN] = {0};
	void __iomem *reg_base;
	u32 lock_id = 0;
	u32 clock_id = 0;
	u32 i;

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] %s node doesn't have clock-output-name property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,ipc-lpm3-cmd-en", &en_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] %s node doesn't have hisilicon,ipc-lpm3-cmd property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,ipc-lpm3-cmd-dis", &dis_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] %s node doesn't have hisilicon,ipc-lpm3-cmd property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_bool(np, "hwspinlock-id")) {
		if (of_property_read_u32_array(np, "hwspinlock-id", &lock_id, 1)) {
			pr_err("[%s] %s node doesn't have hwspinlock-id property!\n", __func__, np->name);
			return;
		}
	}
	if (of_property_read_bool(np, "clock-id")) {
		if (of_property_read_u32_array(np, "clock-id", &clock_id, 1)) {
			pr_err("[%s] %s clock_id property is null!\n",
				 __func__, np->name);
		}
	}
	parent_names = of_clk_get_parent_name(np, 0);

	ppll_clk = kzalloc(sizeof(struct hi3xxx_ppll_clk), GFP_KERNEL);
	if (!ppll_clk) {
		pr_err("[%s] fail to alloc pclk!\n", __func__);
		goto err_ppll_clk;
	}
	/* initialize the reference count */
	ppll_clk->ref_cnt = 0;
	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);
	init->ops = &hi3xxx_ppll_ops;
	init->flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED;
	init->parent_names = &parent_names;
	init->num_parents = 1;

	for (i = 0; i < LPM3_CMD_LEN; i++) {
		ppll_clk->en_cmd[i] = en_cmd[i];
		ppll_clk->dis_cmd[i] = dis_cmd[i];
	}
	ppll_clk->lock = &hs_clk.lock;
	ppll_clk->hw.init = init;
	ppll_clk->addr = reg_base;
	ppll_clk->sctrl = NULL;
	ppll_clk->endisable_addr = hs_clk_base(HS_CRGCTRL);
	ppll_clk->flags = lock_id;
	ppll_clk->clock_id = clock_id;
	ppll_clk->clk_hwlock = NULL;

	clk = clk_register(NULL, &ppll_clk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to reigister clk %s!\n",
			__func__, clk_name);
		goto err_clk;
	}

	clk_register_clkdev(clk, clk_name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;/*lint !e429*/
err_clk:
	iounmap(ppll_clk->sctrl);
	ppll_clk->sctrl = NULL;
	kfree(init);
	init = NULL;
err_init:
	kfree(ppll_clk);
	ppll_clk = NULL;
err_ppll_clk:
	return;
}

static int __init hi3xxx_parse_mux(struct device_node *np,
				   u8 *num_parents)
{
	int i, cnt;

	/* get the count of items in mux */
	for (i = 0, cnt = 0;; i++, cnt++) {
		/* parent's #clock-cells property is always 0 */
		if (!of_parse_phandle(np, "clocks", i))
			break;
	}

	for (i = 0; i < cnt; i++) {
		if (!of_clk_get_parent_name(np, i)) {
			pr_err("[%s] cannot get %dth parent_clk name!\n",
				__func__, i);
			return -ENOENT;
		}
	}
	*num_parents = cnt;

	return 0;
}

static void __init hi3xxx_clkmux_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name, **parent_names = NULL;
	u32 rdata[2] = {0};
	u32 width = 0;
	u8 num_parents, shift, flag = 0;
	void __iomem *reg, *base;
	int i, ret;

	base = hs_clk_get_base(np);
	if (!base) {
		pr_err("[%s] fail to get base!\n", __func__);
		return;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] %s node doesn't have clock-output-name property!\n",
				__func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,clkmux-reg",
				       &rdata[0], 2)) {
		pr_err("[%s] %s node doesn't have clkmux-reg property!\n",
				__func__, np->name);
		return;
	}

	if (of_property_read_bool(np, "hiword"))
		flag = CLK_MUX_HIWORD_MASK;
	ret = hi3xxx_parse_mux(np, &num_parents);
	if (ret) {
		pr_err("[%s] %s node cannot get num_parents!\n",
			__func__, np->name);
		return;
	}
	parent_names = kzalloc(sizeof(char *) * num_parents, GFP_KERNEL);
	if (!parent_names) {
		pr_err("[%s] fail to alloc parent_names!\n", __func__);
		return;
	}
	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(np, i);
	reg = base + rdata[0];
	shift = ffs(rdata[1]) - 1;
	width = fls(rdata[1]) - ffs(rdata[1]) + 1;

	clk = clk_register_mux(NULL, clk_name, parent_names, num_parents,
				     CLK_SET_RATE_PARENT, reg, shift, width,
				     flag, &hs_clk.lock);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to register mux clk %s!\n",
			__func__, clk_name);
		goto err_clk;
	}
	clk_register_clkdev(clk, clk_name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	kfree(parent_names);
	parent_names = NULL;
	return;
err_clk:
	kfree(parent_names);
	parent_names = NULL;
	return;
}

static void __init hs_clkgate_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name, *parent_names;
	unsigned long flags = 0;
	void __iomem *reg_base;
	u32 data[2] = {0};

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}
	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] node %s doesn't have clock-output-names property!\n",
			__func__, np->name);
		return;
	}
	if (of_property_read_u32_array(np, "hisilicon,clkgate",
				       &data[0], 2)) {
		pr_err("[%s] node %s doesn't have clkgate property!\n",
			__func__, np->name);
		return;
	}
	if (of_property_read_bool(np, "hisilicon,clkgate-inverted"))
		flags = CLK_GATE_SET_TO_DISABLE;
	if (of_property_read_bool(np, "hiword"))
		flags |= CLK_GATE_HIWORD_MASK;
	if (of_property_read_bool(np, "always_on"))
		flags |= CLK_GATE_ALWAYS_ON_MASK;
	if (of_property_read_bool(np, "pmu32khz"))
		data[0] = data[0] << 2;

	/* gate only has the fixed parent */
	if (IS_FPGA()) {
		if (NULL != of_find_property(np, "clock-fpga", NULL)) {
			if (of_property_read_string(np, "clock-fpga", &parent_names)) {
				pr_err("[%s] %s node clock-fpga value is NULL!\n", __func__, np->name);
				return;
			}
		} else {
			parent_names = of_clk_get_parent_name(np, 0);
		}
	} else {
		parent_names = of_clk_get_parent_name(np, 0);
	}

	clk = clk_register_gate(NULL, clk_name, parent_names,
				CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, reg_base + data[0],
				(u8)data[1], flags, &hs_clk.lock);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to register gate clk %s!\n",
			__func__, clk_name);
		goto err;
	}

	clk_register_clkdev(clk, clk_name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;
err:
	return;
}

static unsigned int hi3xxx_get_table_maxdiv(const struct clk_div_table *table)
{
	unsigned int maxdiv = 0;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div > maxdiv)
			maxdiv = clkt->div;
	return maxdiv;
}

static unsigned int hi3xxx_get_table_div(const struct clk_div_table *table,
							unsigned int val)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == val)
			return clkt->div;
	return 0;
}

static unsigned int hi3xxx_get_table_val(const struct clk_div_table *table,
					 unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}

static unsigned long hi3xxx_clkdiv_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct hi3xxx_divclk *dclk = container_of(hw, struct hi3xxx_divclk, hw);
	unsigned int div, val;

	val = readl(dclk->reg) >> dclk->shift;
	val &= WIDTH_TO_MASK(dclk->width);

	div = hi3xxx_get_table_div(dclk->table, val);
	if (!div) {
		pr_warn("%s: Invalid divisor for clock %s\n", __func__,
			   __clk_get_name(hw->clk));
		return parent_rate;
	}

	return parent_rate / div;
}

static bool hi3xxx_is_valid_table_div(const struct clk_div_table *table,
				      unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return true;
	return false;
}

static int hi3xxx_clkdiv_bestdiv(struct clk_hw *hw, unsigned long rate,
				 unsigned long *best_parent_rate)
{
	struct hi3xxx_divclk *dclk = container_of(hw, struct hi3xxx_divclk, hw);
	struct clk *clk_parent = __clk_get_parent(hw->clk);
	int i, bestdiv = 0;
	unsigned long parent_rate, best = 0, now, maxdiv;

	maxdiv = hi3xxx_get_table_maxdiv(dclk->table);

	if (!(__clk_get_flags(hw->clk) & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = DIV_ROUND_UP(parent_rate, rate);
		bestdiv = bestdiv == 0 ? 1 : bestdiv;
		bestdiv = bestdiv > maxdiv ? maxdiv : bestdiv;/*lint !e574*/
		return bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * i below
	 */
	maxdiv = min(ULONG_MAX / rate, maxdiv);

	for (i = 1; i <= maxdiv; i++) {/*lint !e574*/
		if (!hi3xxx_is_valid_table_div(dclk->table, i))
			continue;
		parent_rate = __clk_round_rate(clk_parent,
					       MULT_ROUND_UP(rate, i));
		now = parent_rate / i;/*lint !e573*/
		if (now <= rate && now > best) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = hi3xxx_get_table_maxdiv(dclk->table);
		*best_parent_rate = __clk_round_rate(clk_parent, 1);
	}

	return bestdiv;
}

static long hi3xxx_clkdiv_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *prate)
{
	int div;

	if (!rate)
		rate = 1;
	div = hi3xxx_clkdiv_bestdiv(hw, rate, prate);

	return *prate / div;/*lint !e573*/
}

static int hi3xxx_clkdiv_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct hi3xxx_divclk *dclk = container_of(hw, struct hi3xxx_divclk, hw);
	unsigned int div, value;
	unsigned long flags = 0;
	u32 data;

	div = parent_rate / rate;
	value = hi3xxx_get_table_val(dclk->table, div);

	if (value > WIDTH_TO_MASK(dclk->width))/*lint !e574*/
		value = WIDTH_TO_MASK(dclk->width);

	if (dclk->lock)
		spin_lock_irqsave(dclk->lock, flags);

	data = readl(dclk->reg);
	data &= ~(WIDTH_TO_MASK(dclk->width) << dclk->shift);/*lint !e502*/
	data |= value << dclk->shift;
	data |= dclk->mbits;
	writel(data, dclk->reg);

	if (dclk->lock)
		spin_unlock_irqrestore(dclk->lock, flags);

	return 0;
}

#ifdef CONFIG_HISI_CLK_DEBUG
static int hi3xxx_divreg_check(struct clk_hw *hw)
{
	unsigned long rate;
	struct clk *clk = hw->clk;
	struct clk *pclk = clk_get_parent(clk);

	rate = hi3xxx_clkdiv_recalc_rate(hw, clk_get_rate(pclk));
	if (rate == clk_get_rate(clk))
		return 1;
	else
		return 0;
}

static void __iomem *hi3xxx_clkdiv_get_reg(struct clk_hw *hw)
{
	struct hi3xxx_divclk *dclk;
	void __iomem	*ret = NULL;
	u32 val = 0;

	dclk = container_of(hw, struct hi3xxx_divclk, hw);

	if (dclk->reg) {
		ret = dclk->reg;
		val = readl(ret);
		val &= dclk->mbits;
		pr_info("\n[%s]: reg = 0x%pK, bits = 0x%x, regval = 0x%x\n",
			__clk_get_name(hw->clk), ret, dclk->mbits, val);
	}

	return ret;
}

static void __iomem *hi3xxx_dumpdiv(struct clk_hw *hw, char* buf)
{
	struct hi3xxx_divclk *dclk;
	void __iomem	*ret = NULL;
	u32 val = 0;
	dclk = container_of(hw, struct hi3xxx_divclk, hw);

	if (dclk->reg && buf) {
		ret = dclk->reg;
		val = readl(ret);
		snprintf(buf, DUMP_CLKBUFF_MAX_SIZE, "[%s] : regAddress = 0x%pK, regval = 0x%x\n", __clk_get_name(hw->clk), dclk->reg, val);
	}
	return 0;
}
#endif

static struct clk_ops hi3xxx_clkdiv_ops = {
	.recalc_rate = hi3xxx_clkdiv_recalc_rate,
	.round_rate = hi3xxx_clkdiv_round_rate,
	.set_rate = hi3xxx_clkdiv_set_rate,
#ifdef CONFIG_HISI_CLK_DEBUG
	.check_divreg = hi3xxx_divreg_check,
	.get_reg = hi3xxx_clkdiv_get_reg,
	.dump_reg = hi3xxx_dumpdiv, /*lint !e64 */
#endif
};

void __init hi3xxx_clkdiv_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name, *parent_names;
	struct clk_init_data *init;
	struct clk_div_table *table;
	struct hi3xxx_divclk *dclk;
	void __iomem *reg_base;
	unsigned int table_num;
	unsigned int i;
	u32 data[2] = {0};
	unsigned int max_div, min_div, multiple;

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] node %s doesn't have clock-output-names property!\n",
			__func__, np->name);
		return;
	}
	/* process the div_table */
	if (of_property_read_u32_array(np, "hisilicon,clkdiv-table",
				       &data[0], 2)) {
		pr_err("[%s] node %s doesn't have clkdiv-table property!\n",
			__func__, np->name);
		return;
	}

	max_div = (u8)data[0];
	min_div = (u8)data[1];

	if (of_property_read_u32_array(np, "hisilicon,clkdiv",
								&data[0], 2)) {
		pr_err("[%s] node %s doesn't have clkdiv property!\n",
			__func__, np->name);
		return;
	}

	table_num = max_div - min_div + 1;

	/* table ends with <0, 0>, so plus one to table_num */
	table = kzalloc(sizeof(struct clk_div_table) * (table_num + 1), GFP_KERNEL);
	if (!table) {
		pr_err("[%s] fail to alloc table!\n", __func__);
		return;
	}

	if (of_property_read_bool(np, "double_div"))
		multiple = 2;
	else
		multiple = 1;

	for (i = 0; i < table_num; i++) {
		table[i].div = (min_div + i) * multiple;
		table[i].val = i;
	}

	if (IS_FPGA()) {
		if (NULL != of_find_property(np, "clock-fpga", NULL)) {
			if (of_property_read_string(np, "clock-fpga", &parent_names)) {
				pr_err("[%s] %s node clock-fpga value is NULL!\n",
									__func__, np->name);
				goto err_dclk;
		}
		} else {
				parent_names = of_clk_get_parent_name(np, 0);
		}
	} else {
		parent_names = of_clk_get_parent_name(np, 0);
	}

	dclk = kzalloc(sizeof(struct hi3xxx_divclk), GFP_KERNEL);
	if (!dclk) {
		pr_err("[%s] fail to alloc dclk!\n", __func__);
		goto err_dclk;
	}
	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);
	init->ops = &hi3xxx_clkdiv_ops;
	init->flags = CLK_SET_RATE_PARENT;
	init->parent_names = &parent_names;
	init->num_parents = 1;

	dclk->reg = reg_base + data[0];
	dclk->shift = ffs(data[1]) - 1;
	dclk->width = fls(data[1]) - ffs(data[1]) + 1;
	dclk->mbits = data[1] << 16;
	dclk->lock = &hs_clk.lock;
	dclk->hw.init = init;
	dclk->table = table;
	clk = clk_register(NULL, &dclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to register div clk %s!\n",
				__func__, clk_name);
		goto err_clk;
	}
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	clk_register_clkdev(clk, clk_name, NULL);
	return;/*lint !e429*/
err_clk:
	kfree(init);
	init = NULL;
err_init:
	kfree(dclk);
	dclk = NULL;
err_dclk:
	kfree(table);
	table = NULL;
	return;
}

static struct device_node *of_get_clk_cpu_node(int cluster)
{
	struct device_node *np = NULL, *parent;
	const u32 *mpidr;

	parent = of_find_node_by_path("/cpus");
	if (!parent) {
		pr_err("failed to find OF /cpus\n");
		return NULL;
	}

	/*
	*Get first cluster node ; big or little custer core0 must
	*contain reg and operating-points node
	*/
	for_each_child_of_node(parent, np) {
		mpidr = of_get_property(np, "reg", NULL);
		if (!mpidr) {
			pr_err("%s missing reg property\n", np->full_name);
			of_node_put(np);
			np = NULL;
			break;
		} else if (((be32_to_cpup(mpidr + 1) >> 8) & 0xff) == cluster) {
			if (of_get_property(np, "operating-points-v2", NULL)) {
				pr_debug("cluster%d suppoet operating-points-v2\n", cluster);
				of_node_put(np);
				break;
			} else if (of_get_property(np, "operating-points", NULL)) {
				pr_debug("cluster%d suppoet operating-points-v1\n", cluster);
				of_node_put(np);
				break;
			} else {
				of_node_put(np);
				pr_err("cluster%d can not find opp v1&v2\n", cluster);
				np = NULL;
			}

			break;
		}
	}

	of_node_put(parent);
	return np;
}

static struct device_node *of_get_xfreq_node(const char *xfreq)
{
	struct device_node *np;

	if (!strcmp(xfreq, "ddrfreq")) {/*lint !e421*/
		np = of_find_compatible_node(NULL, NULL, "hisilicon,ddr_devfreq");
	} else if (!strcmp(xfreq, "gpufreq")) {/*lint !e421*/
		np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	} else if (!strcmp(xfreq, "sysctrl")) {/*lint !e421*/
		np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	} else {
		return NULL;
	}
	return np;
}


int xfreq_clk_table_init(struct device_node *np, struct hi3xxx_xfreq_clk *xfreqclk)
{
	int count = 0, ret = 0, k = 0;
	unsigned long long rate;
	unsigned int volt, freq;
	struct device_node *opp_np, *opp_np_chd;
	const struct property *prop;
	const __be32 *val;
	int nr;

	opp_np = of_parse_phandle(np, "operating-points-v2", 0);

	if (opp_np) {
		for_each_available_child_of_node(opp_np, opp_np_chd) {
			count++;
			ret = of_property_read_u64(opp_np_chd, "opp-hz", &rate);
			if (ret) {
				pr_err("%s: Failed to read opp-hz, %d\n", __func__,
					ret);
				return ret;
			}
			ret = of_property_read_u32(opp_np_chd, "opp-microvolt", &volt);
			if (ret) {
				pr_err("%s: Failed to read  opp-microvolt, %d\n", __func__,
					ret);
				return ret;
			}
			xfreqclk->freq[k] = (unsigned int)(rate / 1000);
			xfreqclk->volt[k] = volt;
			k++;
		}
		/* There should be one of more OPP defined */
		if (WARN_ON(!count)) //lint !e730
			return -ENOENT;
		return ret;
	} else {
		prop = of_find_property(np, "operating-points", NULL);
		if (!prop)
			return -ENODEV;
		if (!prop->value)
			return -ENODATA;

		/*
		 * Each OPP is a set of tuples consisting of frequency and
		 * voltage like <freq-kHz vol-uV>.
		 */
		nr = prop->length / sizeof(u32);/*lint !e573*/
		if ((nr % 2) || (nr / 2) > MAX_FREQ_NUM) {
			pr_err("%s: Invalid OPP list\n", __func__);
			return -EINVAL;
		}

		xfreqclk->table_length = nr / 2;
		val = prop->value;
		while (nr) {
			freq = be32_to_cpup(val++);
			volt = be32_to_cpup(val++);
			xfreqclk->freq[k] = freq;
			xfreqclk->volt[k] = volt;
			pr_debug("[%s]: the OPP k %d,freq %d\n", __func__, k, freq);
			nr -= 2;
			k++;
		}

		return 0;
	}

}

#define FREQ_INDEX_MASK		0xF
static unsigned int hi3xxx_xfreq_clk_get_freq_idx(struct hi3xxx_xfreq_clk *xfreq_clk)
{
	unsigned int sys_bak_reg;

	if (!xfreq_clk->reg)
		return 0;

	sys_bak_reg = readl(xfreq_clk->reg);

	/*sysctrl SCBAKDATA4
		bit 0-3		LITTLE Cluster
		bit 4-7		BIG Cluster
		bit 8-11	DDR
		bit 12-15	GPU*/
	switch (xfreq_clk->id) {
	case 0:
		break;
	case 1:
		sys_bak_reg >>= 4;
		break;
	case 2:
		sys_bak_reg >>= 12;
		break;
	case 3:
		sys_bak_reg >>= 8;
		break;
	default:
		return 0;
	}

	sys_bak_reg &= FREQ_INDEX_MASK;

	if (sys_bak_reg >= xfreq_clk->table_length)
		sys_bak_reg = 0;

	return sys_bak_reg;
}

static unsigned long hi3xxx_xfreq_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct hi3xxx_xfreq_clk *xfreq_clk = container_of(hw, struct hi3xxx_xfreq_clk, hw);
	u32 rate;
	unsigned int freq_index;

	switch (xfreq_clk->id) {
	/* DDR get freq */
	case 3:
		freq_index = hi3xxx_xfreq_clk_get_freq_idx(xfreq_clk);
		rate = xfreq_clk->freq[freq_index]*1000;
		pr_debug("[%s]3 idx=%d rate=%d\n", __func__, freq_index, rate);
		break;
	/* DDR set min */
	case 4:
	default:
		rate = xfreq_clk->rate;
	}

	return rate;
}

static long hi3xxx_xfreq_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *prate)
{
	return rate;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
static long hi3xxx_xfreq_clk_determine_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long *best_parent_rate,
					struct clk **best_parent_clk)
{
	return rate;
}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static int hi3xxx_xfreq_clk_determine_rate(struct clk_hw *hw,
			     struct clk_rate_request *req)
{
	return 0;
}
#else
static long hi3xxx_xfreq_clk_determine_rate(struct clk_hw *hw, unsigned long rate,
					  unsigned long min_rate, unsigned long max_rate,
					  unsigned long *best_parent_rate, struct clk_hw **best_parent_hw)
{
	return rate;
}
#endif

static int hi3xxx_xfreq_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct hi3xxx_xfreq_clk *xfreq_clk = container_of(hw, struct hi3xxx_xfreq_clk, hw);
	unsigned long new_rate = rate/1000000;
	int ret = 0;

	pr_debug("[%s] set rate = %ldMHZ\n", __func__, new_rate);
	xfreq_clk->set_rate_cmd[1] = new_rate;
#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
	ret = hisi_clkmbox_send_msg(xfreq_clk->set_rate_cmd);
	if (ret < 0) {
		pr_err("[%s]core id:%d fail to send msg to LPM3!\n",
					__func__, xfreq_clk->id);

		return -EINVAL;
	}
#endif

	xfreq_clk->rate = rate;
	return ret;
}

static struct clk_ops hi3xxx_xfreq_clk_ops = {
	.recalc_rate = hi3xxx_xfreq_clk_recalc_rate,
	.determine_rate = hi3xxx_xfreq_clk_determine_rate,
	.round_rate = hi3xxx_xfreq_clk_round_rate,
	.set_rate = hi3xxx_xfreq_clk_set_rate,
};

/*
 * xfreq_clk is used for cpufreq & devfreq.
 */
void __init hi3xxx_xfreq_clk_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name, *parent_names;
	struct clk_init_data *init;
	struct hi3xxx_xfreq_clk *xfreqclk;
	u32 get_rate_cmd[LPM3_CMD_LEN], set_rate_cmd[LPM3_CMD_LEN] = {0};
	u32 device_id = 0, i = 0;
	u32 scbacdata = 0;
	void __iomem *reg_base;
	struct device_node *xfreq_np;
	int k, temp = 0;
	unsigned int freq_index;

	reg_base = hs_clk_get_base(np);
	if (!reg_base) {
		pr_err("[%s] fail to get reg_base!\n", __func__);
		return;
	}

	if (NULL == of_clk_get_parent_name(np, 0))
		parent_names = NULL;
	else
		parent_names = of_clk_get_parent_name(np, 0);

	if (of_property_read_u32(np, "hisilicon,hi3xxx-xfreq-devid", &device_id)) {
		pr_err("[%s] node %s doesn't have clock-output-names property!\n",
			__func__, np->name);
		goto err_prop;
	}

	if (of_property_read_u32_array(np, "hisilicon,get-rate-ipc-cmd", &get_rate_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] node %s doesn't get-rate-ipc-cmd property!\n",
			__func__, np->name);
		goto err_prop;
	}

	if (of_property_read_u32_array(np, "hisilicon,set-rate-ipc-cmd", &set_rate_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] node %s doesn't set-rate-ipc-cmd property!\n",
			__func__, np->name);
		goto err_prop;
	}
	if (of_property_read_u32(np, "hisilicon,hi3xxx-xfreq-scbakdata", &scbacdata)) {
		pr_err("[%s] node %s doesn't hi3xxx-xfreq-scbakdata property!\n",
			__func__, np->name);
		goto err_prop;
	}

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] node %s doesn't have clock-output-names property!\n",
			__func__, np->name);
		goto err_prop;
	}
	xfreqclk = kzalloc(sizeof(struct hi3xxx_xfreq_clk), GFP_KERNEL);
	if (!xfreqclk) {
		pr_err("[%s] fail to alloc xfreqclk!\n", __func__);
		goto err_prop;
	}
	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);
	init->ops = &hi3xxx_xfreq_clk_ops;
	init->parent_names = (parent_names ? &parent_names : NULL);
	init->num_parents = (parent_names ? 1 : 0);
	init->flags = CLK_IS_ROOT | CLK_GET_RATE_NOCACHE;
	xfreq_np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	xfreqclk->reg = of_iomap(xfreq_np, 0);
	xfreqclk->hw.init = init;
	xfreqclk->id = device_id;

	if (xfreqclk->reg == NULL) {
		pr_err("[%s] iomap fail!\n", __func__);
		goto err_clk;
	}

	xfreqclk->reg += scbacdata;

	for (i = 0; i < LPM3_CMD_LEN; i++) {
		xfreqclk->set_rate_cmd[i] = set_rate_cmd[i];
		xfreqclk->get_rate_cmd[i] = get_rate_cmd[i];
	}

	switch (device_id) {
	case 0:
	case 1:
		xfreq_np = of_get_clk_cpu_node(device_id);
		if (!xfreq_clk_table_init(xfreq_np, xfreqclk)) {
			freq_index = hi3xxx_xfreq_clk_get_freq_idx(xfreqclk);
			xfreqclk->rate = xfreqclk->freq[freq_index] * 1000;
		}
		break;
	case 2:
		xfreq_np = of_get_xfreq_node("gpufreq");
		if (!xfreq_clk_table_init(xfreq_np, xfreqclk)) {
			freq_index = hi3xxx_xfreq_clk_get_freq_idx(xfreqclk);
			xfreqclk->rate = xfreqclk->freq[freq_index] * 1000;
		}
		break;
	case 3:
	case 4:
	case 5:
		xfreq_np = of_get_xfreq_node("ddrfreq");
		xfreq_clk_table_init(xfreq_np, xfreqclk);
		/*sort lowtohigh*/
		for (i = 0; i < MAX_FREQ_NUM - 1; i++) {
			for (k = MAX_FREQ_NUM - 1; k > i; k--) {/*lint !e574*/
				if (xfreqclk->freq[k] < xfreqclk->freq[k-1]) {
					temp = xfreqclk->freq[k];
					xfreqclk->freq[k] = xfreqclk->freq[k-1];
					xfreqclk->freq[k-1] = temp;
				}
			}
		}

		k = 0;
		for (i = 0; i < MAX_FREQ_NUM; i++) {
			pr_debug("1xfreqclk->freq[i]=%d\n", xfreqclk->freq[i]);
			if (xfreqclk->freq[i] == 0) {
				k++;
				continue;
			}
			xfreqclk->freq[i-k] = xfreqclk->freq[i];
		}
		if (k > 0) {
			for (i = MAX_FREQ_NUM - 1; i > MAX_FREQ_NUM - k - 1; i--)/*lint !e574*/
				xfreqclk->freq[i] = 0;
		}
		for (i = 0; i < MAX_FREQ_NUM; i++)
			pr_debug("2xfreqclk->freq[i]=%d\n", xfreqclk->freq[i]);
		break;
	default:
		pr_err("[%s]dev_id is error!\n", __func__);
		goto err_clk;
	}


	clk = clk_register(NULL, &xfreqclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to register xfreqclk %s!\n",
				__func__, clk_name);
		goto err_clk;
	}

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	clk_register_clkdev(clk, clk_name, NULL);
	return;

err_clk:
	kfree(init);
	init = NULL;
err_init:
	kfree(xfreqclk);
	xfreqclk = NULL;
err_prop:
	return;
}

static int hi3xxx_mclk_prepare(struct clk_hw *hw)
{
	struct hi3xxx_mclk *mclk;
#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
	s32 ret;
#endif
	mclk = container_of(hw, struct hi3xxx_mclk, hw);
	mclk->ref_cnt++;

#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
	/* notify m3 when the ref_cnt of mclk is 1 */
	if (mclk->gate_abandon_enable)
		return 0;

	if (CLK_DVFS_IPC_CMD == mclk->en_cmd[1]) {
		g_count_num_dvfs++;
		if (1 == g_count_num_dvfs) {
			ret = hisi_clkmbox_send_msg_sync(&mclk->en_cmd[0]);
			if (ret)
				pr_err("[%s] fail to enable clk, ret = %d!\n", __func__, ret);
		}
	} else {
		if (1 == mclk->ref_cnt) {
			ret = hisi_clkmbox_send_msg_sync(&mclk->en_cmd[0]);
			if (ret)
				pr_err("[%s] fail to enable clk, ret = %d!\n", __func__, ret);
		}
	}
#endif

	return 0;
}

static void hi3xxx_mclk_unprepare(struct clk_hw *hw)
{
	struct hi3xxx_mclk *mclk;
#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
	s32 ret;
#endif
	mclk = container_of(hw, struct hi3xxx_mclk, hw);
	mclk->ref_cnt--;

#ifdef CONFIG_HISI_CLK_MAILBOX_SUPPORT
	/* notify m3 when the ref_cnt of gps_clk is 0 */
	if (mclk->always_on)
		return;
	if (CLK_DVFS_IPC_CMD == mclk->dis_cmd[1]) {
		g_count_num_dvfs--;
		if (!g_count_num_dvfs) {
			ret = hisi_clkmbox_send_msg_sync(&mclk->dis_cmd[0]);
			if (ret)
				pr_err("[%s] fail to disable clk, ret = %d!\n", __func__, ret);
		}
	} else {
		if (!mclk->ref_cnt) {
			ret = hisi_clkmbox_send_msg_sync(&mclk->dis_cmd[0]);
			if (ret)
				pr_err("[%s] fail to disable clk, ret = %d!\n", __func__, ret);
		}
	}
#endif
}

static struct clk_ops hi3xxx_mclk_ops = {
	.prepare	= hi3xxx_mclk_prepare,
	.unprepare	= hi3xxx_mclk_unprepare,
};

static void __init hi3xxx_mclk_setup(struct device_node *np)
{
	struct hi3xxx_mclk *mclk;
	struct clk_init_data *init;
	struct clk *clk;
	const char *clk_name, *parent_names;
	u32 en_cmd[LPM3_CMD_LEN] = {0};
	u32 dis_cmd[LPM3_CMD_LEN] = {0};
	u32 clock_id = 0;
	u32 i;

	if (of_property_read_string(np, "clock-output-names", &clk_name)) {
		pr_err("[%s] %s node doesn't have clock-output-name property!\n",
			 __func__, np->name);
		return;
	}

	if (of_property_read_u32_array(np, "hisilicon,ipc-lpm3-cmd-en", &en_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] %s node doesn't have hisilicon,ipc-modem-cmd property!\n",
			 __func__, np->name);
		return;
	}

	if (of_property_read_u32_array(np, "hisilicon,ipc-lpm3-cmd-dis", &dis_cmd[0], LPM3_CMD_LEN)) {
		pr_err("[%s] %s node doesn't have hisilicon,ipc-modem-cmd property!\n",
			 __func__, np->name);
		return;
	}
	if (of_property_read_bool(np, "clock-id")) {
		if (of_property_read_u32_array(np, "clock-id", &clock_id, 1)) {
			pr_err("[%s] %s clock_id property is null!\n",
				 __func__, np->name);
		}
	}

	parent_names = of_clk_get_parent_name(np, 0);

	mclk = kzalloc(sizeof(struct hi3xxx_mclk), GFP_KERNEL);
	if (!mclk) {
		pr_err("[%s] fail to alloc pclk!\n", __func__);
		goto err_mclk;
	}

	init = kzalloc(sizeof(struct clk_init_data), GFP_KERNEL);
	if (!init) {
		pr_err("[%s] fail to alloc init!\n", __func__);
		goto err_init;
	}
	init->name = kstrdup(clk_name, GFP_KERNEL);
	init->ops = &hi3xxx_mclk_ops;
	init->flags = CLK_SET_RATE_PARENT;
	init->parent_names = &parent_names;
	init->num_parents = 1;

	for (i = 0; i < LPM3_CMD_LEN; i++)
		mclk->en_cmd[i] = en_cmd[i];
	for (i = 0; i < LPM3_CMD_LEN; i++)
		mclk->dis_cmd[i] = dis_cmd[i];

	if (of_property_read_bool(np, "always_on"))
		mclk->always_on = 1;
	else
		mclk->always_on = 0;
	if (of_property_read_bool(np, "gate_abandon_enable"))
		mclk->gate_abandon_enable = 1;
	else
		mclk->gate_abandon_enable = 0;
	/* initialize the reference count */
	mclk->ref_cnt = 0;
	mclk->lock = &hs_clk.lock;
	mclk->clock_id = clock_id;
	mclk->hw.init = init;

	clk = clk_register(NULL, &mclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to reigister clk %s!\n",
			__func__, clk_name);
		goto err_clk;
	}

	clk_register_clkdev(clk, clk_name, NULL);
	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;/*lint !e429*/

err_clk:
	kfree(init);
	init = NULL;
err_init:
	kfree(mclk);
	mclk = NULL;
err_mclk:
	return;
}
/*lint -save -e611*/
CLK_OF_DECLARE(hi3xxx_mux, "hisilicon,hi3xxx-clk-mux", hi3xxx_clkmux_setup);
CLK_OF_DECLARE(hi3xxx_div, "hisilicon,hi3xxx-clk-div", hi3xxx_clkdiv_setup);
CLK_OF_DECLARE(hs_gate, "hisilicon,clk-gate", hs_clkgate_setup);
CLK_OF_DECLARE(hi3xxx_pmu_gate, "hisilicon,clk-pmu-gate", hi3xxx_pmu_clkgate_setup);
CLK_OF_DECLARE(hi3xxx_gate, "hisilicon,hi3xxx-clk-gate", hi3xxx_clkgate_setup);
CLK_OF_DECLARE(hi3xxx_ppll, "hisilicon,ppll-ctrl", hi3xxx_ppll_setup);
CLK_OF_DECLARE(hi3xxx_cpu, "hisilicon,hi3xxx-xfreq-clk", hi3xxx_xfreq_clk_setup);
CLK_OF_DECLARE(hi3xxx_mclk, "hisilicon,interactive-clk", hi3xxx_mclk_setup);
/*lint -restore*/
static const struct of_device_id hs_of_match[] = {
	{ .compatible = "hisilicon,clk-pmctrl",	.data = (void *)HS_PMCTRL, },
	{ .compatible = "hisilicon,clk-sctrl",	.data = (void *)HS_SYSCTRL, },
	{ .compatible = "hisilicon,clk-crgctrl",	.data = (void *)HS_CRGCTRL, },
	{ .compatible = "hisilicon,hi6421pmic",	.data = (void *)HS_PMUCTRL, },
	{ .compatible = "hisilicon,clk-pctrl",	.data = (void *)HS_PCTRL, },
	{ .compatible = "hisilicon,media-crg",	.data = (void *)HS_MEDIACRG, },
	{ .compatible = "hisilicon,iomcu-crg",	.data = (void *)HS_IOMCUCRG, },
	{ .compatible = "hisilicon,media1-crg",	.data = (void *)HS_MEDIA1CRG, },
	{ .compatible = "hisilicon,media2-crg",	.data = (void *)HS_MEDIA2CRG, },
	{},/*lint !e785 */
};

void __iomem __init *hs_clk_base(u32 ctrl)
{
	struct device_node *np;
	void __iomem *ret = NULL;

	switch (ctrl) {
	case HS_SYSCTRL:
		np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
		break;
	case HS_CRGCTRL:
		np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
		break;
	case HS_PMCTRL:
		np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
		break;
	case HS_MEDIACRG:
		np = of_find_compatible_node(NULL, NULL, "hisilicon,mediactrl");
		break;
	default:
		pr_err("[%s] ctrl err!\n", __func__);
		BUG_ON(1);
	}
	if (!np) {
		pr_err("[%s] node doesn't have node!\n", __func__);
		goto out;
	}
	ret = of_iomap(np, 0);
	WARN_ON(!ret);
out:
	return ret;
}

static void __iomem __init *hs_clk_get_base(struct device_node *np)
{
	struct device_node *parent;
	const struct of_device_id *match;
	void __iomem *ret = NULL;

	parent = of_get_parent(np);
	if (!parent) {
		pr_err("[%s] node %s doesn't have parent node!\n", __func__, np->name);
		goto out;
	}

	match = of_match_node(hs_of_match, parent);
	if (!match) {
		pr_err("[%s] parent node %s doesn't match!\n", __func__, parent->name);
		goto out;
	}
	switch ((unsigned long)match->data) {
	case HS_PMCTRL:
		if (!hs_clk.pmctrl) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);
			hs_clk.pmctrl = ret;
		} else {
			ret = hs_clk.pmctrl;
		}
		break;
	case HS_SYSCTRL:
		if (!hs_clk.sctrl) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);
			hs_clk.sctrl = ret;
		} else {
			ret = hs_clk.sctrl;
		}
		break;
	case HS_CRGCTRL:
		if (!hs_clk.crgctrl) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);
			hs_clk.crgctrl = ret;
		} else {
			ret = hs_clk.crgctrl;
		}
		break;
	case HS_PMUCTRL:
		if (!hs_clk.pmuctrl) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);
			hs_clk.pmuctrl = ret;
		} else {
			ret = hs_clk.pmuctrl;
		}
		break;
	case HS_PCTRL:
		if (!hs_clk.pctrl) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);
			hs_clk.pctrl = ret;
		} else {
			ret = hs_clk.pctrl;
		}
		break;
	case HS_MEDIACRG:
		if(!hs_clk.mediacrg) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);/*lint !e730 */
			hs_clk.mediacrg = ret;
		} else {
			ret = hs_clk.mediacrg;
		}
		break;
	case HS_IOMCUCRG:
		if(!hs_clk.iomcucrg) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);/*lint !e730 */
			hs_clk.iomcucrg = ret;
		} else {
			ret = hs_clk.iomcucrg;
		}
		break;
	case HS_MEDIA1CRG:
		if(!hs_clk.media1crg) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);/*lint !e730 */
			hs_clk.media1crg = ret;
		} else {
			ret = hs_clk.media1crg;
		}
		break;
	case HS_MEDIA2CRG:
		if(!hs_clk.media2crg) {
			ret = of_iomap(parent, 0);
			WARN_ON(!ret);/*lint !e730 */
			hs_clk.media2crg = ret;
		}else{
			ret = hs_clk.media2crg;
		}
		break;

	default:
		pr_err("[%s] cannot find the match node!\n", __func__);
		ret = NULL;
	}
out:
	return ret;
}
