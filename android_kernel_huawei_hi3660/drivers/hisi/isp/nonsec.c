/*
 * hisilicon driver, hisi_isp_rproc.c
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 */

/*lint -e747 -e715
 -esym(747,*) -esym(715,*)*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/rproc_share.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <linux/iommu.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/miscdevice.h>
#include "hisp_internel.h"
#include "isp_ddr_map.h"

#define ISP_MEM_SIZE    0x10000
#define TMP_SIZE        0x1000
#define MAX_SIZE        64

#define CRGPERIPH_PERPWRSTAT_ADDR       (0X158)

struct hisp_pwr_ops {
    struct mutex lock;
    unsigned int refs_a7;
    unsigned int refs_isp;
    unsigned int refs_ispinit;
    int (*ispup)(struct hisp_pwr_ops *);
    int (*ispdn)(struct hisp_pwr_ops *);
    int (*ispinit)(struct hisp_pwr_ops *);
    int (*ispexit)(struct hisp_pwr_ops *);
    int (*a7up)(struct hisp_pwr_ops *);
    int (*a7dn)(struct hisp_pwr_ops *);
};

struct hisi_isp_nsec {
    struct device *device;
    struct platform_device *isp_pdev;
    struct hisp_pwr_ops *isp_ops;
    struct regulator *clockdep_supply;
    struct regulator *ispcore_supply;
    unsigned int clock_num;
    struct clk *ispclk[ISP_CLK_MAX];
    unsigned int ispclk_value[ISP_CLK_MAX];
    unsigned int clkdis_dvfs[ISP_CLK_MAX];
    unsigned int clkdis_need_div[ISP_CLK_MAX];
    unsigned int clkdn[HISP_CLKDOWN_MAX][ISP_CLK_MAX];
    const char *clk_name[ISP_CLK_MAX];
    unsigned int dvfsmask;
    void *isp_dma_va;
    dma_addr_t isp_dma;
    u64 pgd_base;
    u64 remap_addr;
    unsigned int useisptop;
    unsigned int useclockdep;
    unsigned int usedvfs;
    unsigned int usepowerdn_clockdn;
};

static struct hisi_isp_nsec nsec_rproc_dev;

#define MEDIASUBSYS_PWSTAT (5)
int get_media1_subsys_power_state(void)
{
    void __iomem* cfg_base = NULL;
    unsigned int media_pw_stat = 0;

    cfg_base = get_regaddr_by_pa(CRGPERI);
    if (cfg_base == NULL) {
        pr_err("[%s] cfg_base remap fail\n", __func__);
        return 0;
    }
    media_pw_stat = __raw_readl((volatile void __iomem*)(CRGPERIPH_PERPWRSTAT_ADDR + cfg_base));

    if (((1 << (unsigned int)MEDIASUBSYS_PWSTAT) & media_pw_stat))
        return 1;
    else
        return 0;
}
#define PERPWRSTAT_ISP (0)
int get_isptop_power_state(void)
{
    void __iomem* cfg_base = NULL;
    unsigned int isppwrstat = 0;

    cfg_base = get_regaddr_by_pa(CRGPERI);
    if (cfg_base == NULL) {
        pr_err("[%s] cfg_base remap fail\n", __func__);
        return 0;
    }
    isppwrstat = __raw_readl((volatile void __iomem*)(CRGPERIPH_PERPWRSTAT_ADDR + cfg_base));

    if (((1 << (unsigned int)PERPWRSTAT_ISP) & isppwrstat))
        return 1;
    else
        return 0;
}

void dump_hisi_isp_boot(struct rproc *rproc,unsigned int size)
{
    return;
}

static int need_powerup(unsigned int refs)
{
    if (refs == 0xffffffff)
        pr_err("[%s] need_powerup exc, refs == 0xffffffff\n", __func__);

    return ((refs == 0) ? 1 : 0);
}

static int need_powerdn(unsigned int refs)
{
    if (refs == 0xffffffff)
        pr_err("[%s] need_powerdn exc, refs == 0xffffffff\n", __func__);

    return ((refs == 1) ? 1 : 0);
}

static void set_isp_nonsec(void)
{
    pr_alert("[%s] +\n", __func__);
    atfisp_set_nonsec();
    pr_alert("[%s] -\n", __func__);
}

static int isptop_power_up(unsigned int useisptop)
{
    int ret = 0;

    if (!useisptop)
        return 0;

    pr_info("[%s] useisptop. %d  +\n", __func__, useisptop);

    if ((ret = atfisp_isptop_power_up()) != 0)
    {
        pr_err("[%s] atfisp_isptop_power_up. %d\n", __func__, ret);
        return ret;
    }
    pr_info("[%s] -\n", __func__);

    return 0;
}

static int isptop_power_down(unsigned int useisptop)
{
    int ret = 0;

    if (!useisptop)
        return 0;

    pr_info("[%s] useisptop. %d  +\n", __func__, useisptop);

    if (useisptop == 0)
    {
        pr_info("[%s] -\n", __func__);
        return 0;
    }
    if ((ret = atfisp_isptop_power_down()) != 0)
    {
        pr_err("[%s] atfisp_isptop_power_down. %d\n", __func__, ret);
        return ret;
    }
    pr_info("[%s] -\n", __func__);

    return 0;
}
static int disreset_ispcpu(u64 remap_addr)
{
    void __iomem *isp_subctrl_base;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    if ((ret = get_media1_subsys_power_state()) == 0) {
        pr_err("[%s] Failed : get_media1_subsys_power_state. %d\n", __func__, ret);
        return -1;
    }
    if ((ret = get_isptop_power_state()) == 0) {
        pr_err("[%s] Failed : get_isptop_power_state. %d\n", __func__, ret);
        return -1;
    }
    isp_subctrl_base = get_regaddr_by_pa(SUBCTRL);
    if (isp_subctrl_base == NULL) {
        pr_err("[%s] Failed : isp_subctrl_base\n",__func__);
        return -1;
    }
    __raw_writel((unsigned int)(ISP_A7_REMAP_ENABLE | (remap_addr >> 16)), (volatile void __iomem *)(isp_subctrl_base + ISP_SUB_CTRL_ISP_A7_CTRL_0));/*lint !e648 */
    if ((ret = atfisp_disreset_ispcpu()) < 0) {
        pr_err("[%s] atfisp_disreset_ispcpu. %d\n", __func__, ret);
        return ret;
    }

    pr_info("[%s] -\n", __func__);
    return 0;
}

static int nsec_ispcpu_powerup(struct hisp_pwr_ops *ops)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    int ret = 0;

    pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);

    if (!need_powerup(ops->refs_a7)) {
        ops->refs_a7++;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_a7);
        return 0;
    }

    /* need config by secure core */
    ret = disreset_ispcpu(dev->remap_addr);
    if (ret < 0) {
        pr_err("[%s] Failed : disreset_ispcpu.%d\n", __func__, ret);
        return ret;
    }

    ops->refs_a7++;
    pr_info("[%s] - refs_a7.0x%x\n", __func__, ops->refs_a7);
    return 0;
}

static int nsec_ispcpu_powerdn(struct hisp_pwr_ops *ops)
{
    pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);

    if (!need_powerdn(ops->refs_a7)) {
        if(ops->refs_a7 > 0)
            ops->refs_a7--;
        pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);
        return 0;
    }

    ops->refs_a7--;
    pr_info("[%s] - refs_a7.0x%x\n", __func__, ops->refs_a7);
    return 0;
}

static int check_clock_valid(int clk)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;

    if (clk >= (int)dev->clock_num) {
        pr_err("[%s] Failed : clk %d >= %d\n", __func__, clk, dev->clock_num);
        return -EINVAL;
    }

    return 0;
}

static int invalid_dvfsmask(int clk)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    int ret = 0;

    if ((ret = check_clock_valid(clk)) < 0)
        return ret;

    return (0x01 & (dev->dvfsmask >> clk));
}

int check_dvfs_valid(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    return dev->usedvfs;
}

static int hisp_clock_down(struct hisi_isp_nsec *dev, int clk, int clkdown)
{
    int ret = 0, stat_machine = 0, type = 0;
    unsigned long ispclk = 0;

    if (!check_dvfs_valid()) {
        pr_err("[%s] Failed : Do not Support DVFS\n", __func__);
        return -EINVAL;
    }

    if (clk >= (int)dev->clock_num) {
        pr_err("[%s] Failed : clk.(%d >= %d)\n", __func__, clk, dev->clock_num);
        return -EINVAL;
    }

    stat_machine = clkdown;
    do {
        type = stat_machine;
        switch (type) {
            case HISP_CLK_TURBO:
            case HISP_CLK_NORMINAL:
            case HISP_CLK_SVS:
            case HISP_CLK_DISDVFS:
                stat_machine ++;
                break;
            default:
                pr_err("[%s] Failed: type.(%d > %d)\n", __func__, type, HISP_CLKDOWN_MAX);
                return -EINVAL;
        }

        ispclk = (unsigned long)dev->clkdn[type][clk];
        pr_info("[%s] Clock Down %lu.%lu MHz\n", __func__, ispclk/1000000, ispclk%1000000);
        if ((ret = clk_set_rate(dev->ispclk[clk], (unsigned long)ispclk)) < 0) {
            pr_err("[%s] Failed: clk_set_rate.%d, %d > %d try clock down ...\n", __func__, ret, type, stat_machine );
            goto try_clock_down;
        }
        if ((ret = clk_prepare_enable(dev->ispclk[clk])) < 0) {
            pr_err("[%s] Failed: clk_prepare_enable.%d, %d > %d try clock down ...\n", __func__, ret, type, stat_machine);
            goto try_clock_down;
        }
        rproc_set_shared_clk_value(clk,(unsigned int)ispclk);
try_clock_down:
        if (ret != 0 && stat_machine < HISP_CLKDOWN_MAX && stat_machine >= 0)
            pr_info("[%s] Try Clock Down %lu.%lu MHz > %u.%u MHz\n", __func__, ispclk/1000000, ispclk%1000000, dev->clkdn[stat_machine][clk]/1000000, dev->clkdn[stat_machine][clk]%1000000);
    } while (ret != 0);

    return ret;
}

static int hisp_clock_enable(struct hisi_isp_nsec *dev, int clk)
{
    unsigned long ispclock = 0;
    int ret;

    if (check_clock_valid(clk))
        return -EINVAL;

    ispclock = (unsigned long)dev->ispclk_value[clk];
    if(ISP_SYS_CLK == clk) {
        if ((ret = clk_prepare_enable(dev->ispclk[clk])) < 0) {
            pr_err("[%s] Failed: %d.%s.clk_prepare_enable.%d\n", __func__, clk, dev->clk_name[clk], ret);
            return -EINVAL;
        }
        pr_info("[%s] %d.%s.clk_prepare_enable\n", __func__, clk, dev->clk_name[clk]);
        return 0;
    }
    if ((ret = clk_set_rate(dev->ispclk[clk], ispclock)) < 0) {
        pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
        goto try_clock_down;
    }

    if ((ret = clk_prepare_enable(dev->ispclk[clk])) < 0) {
        pr_err("[%s] Failed: %d.%d M, %d.%s.clk_prepare_enable.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
        goto try_clock_down;
    }
    rproc_set_shared_clk_value(clk,(unsigned int)ispclock);
    pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__, clk, dev->clk_name[clk], (int)ispclock/1000000, (int)ispclock%1000000);

    return 0;

try_clock_down:
    return hisp_clock_down(dev, clk, HISP_CLK_SVS);
}

static void hisp_clock_disable(struct hisi_isp_nsec *dev, int clk)
{
    unsigned long ispclock = 0;
    int ret = 0;

    if (check_clock_valid(clk))
        return;

    if (dev->usepowerdn_clockdn) {
        switch (clk) {
            case ISPCPU_CLK:
            case ISPI2C_CLK:
            case ISPFUNC_CLK:
            case ISPFUNC2_CLK:
            case ISPFUNC3_CLK:
            case ISPFUNC4_CLK:
                ispclock = (unsigned long)dev->clkdis_need_div[clk];
                if (ispclock != 0) {
                    if ((ret = clk_set_rate(dev->ispclk[clk], ispclock)) < 0) {
                        pr_err("[%s] Failed: need_div %d.%d M, %d.%s.clk_set_rate.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
                        return;
                    }
                    pr_info("[%s] %d.%s.need_div clk_set_rate.%d.%d M\n", __func__, clk, dev->clk_name[clk], (int)ispclock/1000000, (int)ispclock%1000000);
                }
                ispclock = (unsigned long)dev->clkdis_dvfs[clk];
                if ((ret = clk_set_rate(dev->ispclk[clk], ispclock)) < 0) {
                    pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
                    return;
                }
                rproc_set_shared_clk_value(clk,(unsigned int)ispclock);
                pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__, clk, dev->clk_name[clk], (int)ispclock/1000000, (int)ispclock%1000000);
                break;
            case ISP_SYS_CLK:
                pr_info("[%s] %d.%s.clk_disable_unprepare\n", __func__, clk, dev->clk_name[clk]);
                break;
            default:
                break;
        }
    }
    clk_disable_unprepare(dev->ispclk[clk]);
}

int secnsec_setclkrate(unsigned int type, unsigned int rate)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    int ret = -EINVAL;

    if (invalid_dvfsmask(type)) {
        pr_err("[%s] Failed : DVFS Invalid type.0x%x, rate.%d, dvfsmask.0x%x\n", __func__, type, rate, dev->dvfsmask);
        return -EINVAL;
    }

    if((rate > dev->ispclk_value[type]) || (rate == 0)) {
        pr_err("[%s] Failed : DVFS type.0x%x.%s, %d.(%d.%08d M) > %d.(%d.%08d M)\n", __func__,
            type, dev->clk_name[type], rate, rate/1000000, rate%1000000, dev->ispclk_value[type], dev->ispclk_value[type]/1000000, dev->ispclk_value[type]%1000000);
        return -EINVAL;
    }

    if ((ret = clk_set_rate(dev->ispclk[type], (unsigned long)rate)) < 0) {
        pr_err("[%s] Failed : DVFS Set.0x%x.%s Rate.%d.(%d.%08d M)\n", __func__, type, dev->clk_name[type], rate, rate/1000000, rate%1000000);
        return ret;
    }
    rproc_set_shared_clk_value((int)type,rate);
    pr_info("[%s] DVFS Set.0x%x.%s Rate.%d.(%d.%08d M)\n", __func__, type, dev->clk_name[type], rate, rate/1000000, rate%1000000);


    return 0;
}

unsigned long secnsec_getclkrate(unsigned int type)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;

    if (invalid_dvfsmask(type)) {
        pr_err("[%s] Failed : DVFS Invalid type.0x%x, dvfsmask.0x%x\n", __func__, type, dev->dvfsmask);
        return 0;
    }

    return (unsigned long)clk_get_rate(dev->ispclk[type]);
}

int nsec_setclkrate(unsigned int type, unsigned int rate)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    mutex_lock(&ops->lock);
    if (ops->refs_a7 == 0) {
        pr_info("[%s] Failed : refs_a7.%d, check ISPCPU PowerDown\n", __func__, ops->refs_a7);
        mutex_unlock(&ops->lock);
        return -ENODEV;
    }

    if ((ret = secnsec_setclkrate(type, rate)) < 0)
        pr_info("[%s] Failed : secnsec_setclkrate.%d\n", __func__, ret);
    mutex_unlock(&ops->lock);

    return ret;
}

int hisp_powerup(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret, err, index, err_index;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    pr_info("[%s] + useclockdep.%d, refs_isp.0x%x\n", __func__, dev->useclockdep, ops->refs_isp);
    if (!need_powerup(ops->refs_isp)) {
        ops->refs_isp++;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_isp);
        return 0;
    }

    if (dev->useclockdep) {
        if ((ret = regulator_enable(dev->clockdep_supply)) != 0) {
            pr_err("[%s] Failed: clockdep regulator_enable.%d\n", __func__, ret);
            return ret;
        }
    }

    for (index = 0; index < (int)dev->clock_num; index ++) {
        if ((ret = hisp_clock_enable(dev, index)) < 0) {
            pr_err("[%s] Failed: hisp_clock_enable.%d, index.%d\n", __func__, ret, index);
            goto err_ispclk;
        }
    }
    if ((ret = regulator_enable(dev->ispcore_supply)) != 0) {
        dump_media1_regs();
        pr_err("[%s] Failed: ispcore regulator_enable.%d\n", __func__, ret);
        goto err_ispclk;
    }

    if ((ret = isptop_power_up(dev->useisptop)) < 0) {
        pr_err("[%s] Failed: isptop_power_up.%d, dev->useisptop.%d\n", __func__, ret, dev->useisptop);
        goto err_isptop;
    }

    ops->refs_isp++;
    pr_info("[%s] - useclockdep.%d, refs_isp.0x%x\n", __func__, dev->useclockdep, ops->refs_isp);

    return 0;

err_isptop:
    if ((err = regulator_disable(dev->ispcore_supply)) != 0)
        pr_err("[%s] Failed: ispsrt regulator_disable.%d, up ret.%d\n", __func__, err, ret);
err_ispclk:
    for (err_index = 0; err_index < index; err_index ++)
        hisp_clock_disable(dev, err_index);

    if (dev->useclockdep) {
        if ((err = regulator_disable(dev->clockdep_supply)) < 0)
            pr_err("[%s] Failed: isp regulator_disable.%d, up ret.%d\n", __func__, err, ret);
    }

    return ret;
}

int hisp_powerdn(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0, index;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    pr_info("[%s] + useclockdep.%d, refs_isp.0x%x\n", __func__, dev->useclockdep, ops->refs_isp);
    if (!need_powerdn(ops->refs_isp)) {
        if(ops->refs_isp > 0)
            ops->refs_isp--;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_isp);
        return 0;
    }

    if ((ret = isptop_power_down(dev->useisptop)) != 0)
        pr_err("[%s] Failed: isptop_power_down.%d useisptop.%d\n", __func__, ret, dev->useisptop);

    if ((ret = regulator_disable(dev->ispcore_supply)) != 0) {
        pr_err("[%s] Failed: ispsrt regulator_disable.%d\n", __func__, ret);
    }

    for (index = 0; index < (int)dev->clock_num; index ++)
        hisp_clock_disable(dev, index);

    if (dev->useclockdep) {
        if ((ret = regulator_disable(dev->clockdep_supply)) != 0)
            pr_err("[%s] Failed: isp regulator_disable.%d\n", __func__, ret);
    }

    ops->refs_isp--;
    pr_info("[%s] - useclockdep.%d, refs_isp.0x%x\n", __func__, dev->useclockdep, ops->refs_isp);

    return 0;
}

static int nsec_isp_powerup(struct hisp_pwr_ops *ops)
{
    return hisp_powerup();
}

static int nsec_isp_powerdn(struct hisp_pwr_ops *ops)
{
    return hisp_powerdn();
}

static int nsec_isp_init(struct hisp_pwr_ops *ops)
{
    int ret = 0;

    pr_info("[%s] + refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);
    if (!need_powerup(ops->refs_ispinit)) {
        ops->refs_ispinit++;
        pr_info("[%s] + refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);
        return 0;
    }

    if ((ret = ispmmu_init()) < 0) {
        pr_err("[%s] Failed: ispmmu_init.%d\n", __func__, ret);
        return ret;
    }

    if ((ret = ispcvdr_init()) < 0) {
        pr_err("[%s] Failed: ispcvdr_init.%d\n", __func__, ret);
        return ret;
    }

    ops->refs_ispinit++;
    pr_err("[%s] - refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);

    return 0;
}

static int nsec_isp_exit(struct hisp_pwr_ops *ops)
{
    int ret;

    pr_info("[%s] + refs_ispinit.%x\n", __func__, ops->refs_ispinit);
    if (!need_powerdn(ops->refs_ispinit)) {
        if(ops->refs_ispinit > 0)
            ops->refs_ispinit--;
        pr_err("[%s] + refs_ispinit.%x\n", __func__, ops->refs_ispinit);
        return 0;
    }

    if ((ret = ispmmu_exit()))
        pr_err("[%s] Failed : ispmmu_exit.%d\n", __func__, ret);

    ops->refs_ispinit--;
    pr_err("[%s] - refs_ispinit.%x\n", __func__, ops->refs_ispinit);

    return 0;
}

static struct hisp_pwr_ops isp_pwr_ops = {
    .lock       = __MUTEX_INITIALIZER(isp_pwr_ops.lock),
    .a7up       = nsec_ispcpu_powerup,
    .a7dn       = nsec_ispcpu_powerdn,
    .ispup      = nsec_isp_powerup,
    .ispdn      = nsec_isp_powerdn,
    .ispinit    = nsec_isp_init,
    .ispexit    = nsec_isp_exit,
};

int hisp_nsec_jpeg_powerup(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -EINVAL;
    }

    mutex_lock(&ops->lock);
    ret = ops->ispup(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispup.%d\n", __func__, ret);
        mutex_unlock(&ops->lock);
        return ret;
    }

    if (need_powerup(ops->refs_ispinit)) {
        ret = ispcpu_qos_cfg();
        if (0 != ret) {
            pr_err("[%s] Failed : ispcpu_qos_cfg.%d\n", __func__, ret);
            goto isp_down;
        }
        set_isp_nonsec();
    }

    ret = ops->ispinit(ops);
    if (0 != ret) {
        pr_err("[%s] ispinit.%d\n", __func__, ret);
        goto isp_down;
    }

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return 0;

isp_down:
    if ((ops->ispdn(ops)) != 0)
        pr_err("[%s] Failed : ispdn\n", __func__);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);
    pr_info("[%s] -\n", __func__);

    return ret;
}
EXPORT_SYMBOL(hisp_nsec_jpeg_powerup);
/*lint -save -e631 -e613*/
int hisp_nsec_jpeg_powerdn(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    pr_info("%s: +\n", __func__);
    if (!ops) {
        pr_err("%s: failed, isp_ops is null.\n", __func__);
    }

    mutex_lock(&ops->lock);

    if ((ret = ops->ispexit(ops)))
        pr_err("%s: jpegdn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispdn(ops)) != 0)
        pr_err("%s: ispdn faled, ret.%d\n", __func__, ret);

    pr_info("%s:refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);

    mutex_unlock(&ops->lock);
    pr_info("%s: -\n", __func__);
    return 0;
}
/*lint -restore */
EXPORT_SYMBOL(hisp_nsec_jpeg_powerdn);

int nonsec_isp_device_enable(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -1;
    }

    mutex_lock(&ops->lock);
    ret = ops->ispup(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispup.%d\n", __func__, ret);
        mutex_unlock(&ops->lock);
        return ret;
    }

    if (need_powerup(ops->refs_ispinit)) {
        ret = ispcpu_qos_cfg();
        if (0 != ret) {
            pr_err("[%s] Failed : ispcpu_qos_cfg.%d\n", __func__, ret);
            goto isp_down;
        }
        set_isp_nonsec();
    }

    ret = ops->ispinit(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispinit.%d\n", __func__, ret);
        goto isp_down;
    }

    ret = ops->a7up(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : a7up.%d\n", __func__, ret);
        goto isp_exit;
    }

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return ret;

isp_exit:
    if ((ops->ispexit(ops)) != 0)
        pr_err("[%s] Failed : ispexit\n", __func__);
isp_down:
    if ((ops->ispdn(ops)) != 0)
        pr_err("[%s] Failed : ispdn\n", __func__);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return ret;
}
EXPORT_SYMBOL(nonsec_isp_device_enable);

int nonsec_isp_device_disable(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -1;
    }

    mutex_lock(&ops->lock);
    if ((ret = ops->a7dn(ops)) != 0)
        pr_err("[%s] a7dn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispexit(ops)))
        pr_err("[%s] jpegdn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispdn(ops)) != 0)
        pr_err("[%s] ispdn faled, ret.%d\n", __func__, ret);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return 0;
}
EXPORT_SYMBOL(nonsec_isp_device_disable);

int hisp_get_pgd_base(u64 *pgd_base)
{
    struct iommu_domain *domain = NULL;
    struct iommu_domain_data *info = NULL;

    if ((domain = hisi_ion_enable_iommu(NULL)) == NULL) {
        pr_err("[%s] Failed : hisi_ion_enable_iommu.%pK\n", __func__, domain);
        return -ENODEV;
    }

    if ((info = (struct iommu_domain_data *)domain->priv) == NULL) {
        pr_err("[%s] Failed : info.%pK\n",__func__, info);
        return -ENODEV;
    }

    *pgd_base = info->phy_pgd_base;
    pr_info("[%s] -\n", __func__);
    return 0;
}

static int set_nonsec_pgd(struct rproc *rproc)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct rproc_shared_para *param = NULL;
    int ret;

    if ((ret = hisp_get_pgd_base(&dev->pgd_base)) < 0) {
        pr_err("[%s] Failed : hisp_get_pgd_base.%d\n", __func__, ret);
        return ret;
    }

    if ((ret = hisi_isp_set_pgd()) < 0) {
        pr_err("[%s] Failed : hisi_isp_set_pgd.%d\n", __func__, ret);
        return ret;
    }

    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if (!param) {
        pr_err("[%s] Failed : param.%pK\n", __func__, param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }
    param->dynamic_pgtable_base = dev->pgd_base;
    hisp_unlock_sharedbuf();

    return 0;
}

int hisi_isp_rproc_pgd_set(struct rproc *rproc)
{
    int err = 0;

    if (use_nonsec_isp()) {
        pr_info("[%s] +\n", __func__);
        err = set_nonsec_pgd(rproc);
        if (0 != err) {
            pr_err("[%s] Failed : set_nonsec_pgd.%d\n", __func__, err);
            return err;
        }
    }
    else{
        struct rproc_shared_para *param = NULL;
        hisp_lock_sharedbuf();
        param = rproc_get_share_para();
        if(param)
            param->dynamic_pgtable_base = get_nonsec_pgd();
        hisp_unlock_sharedbuf();
    }

    return 0;
}

static int hisi_isp_nsec_getdts_dvfs(struct device_node *np, struct hisi_isp_nsec *dev)
{
    int ret;

    if (dev->usedvfs) {
        if ((ret = of_property_read_u32_array(np, "clock-value", dev->clkdn[HISP_CLK_TURBO], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: TURBO of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdn-normal", dev->clkdn[HISP_CLK_NORMINAL], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: NORMINAL of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdn-svs", dev->clkdn[HISP_CLK_SVS], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: SVS of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdis-dvfs", dev->clkdn[HISP_CLK_DISDVFS], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: SVS of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }
    }

    return 0;
}

static int hisi_isp_nsec_getdts(struct platform_device *pdev, struct hisi_isp_nsec *dev)
{
    struct device *device = &pdev->dev;
    struct device_node *np = device->of_node;
    int ret, i;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -ENODEV;
    }

    pr_info("[%s] +\n", __func__);
    dev->device = device;
    dev->clockdep_supply = devm_regulator_get(device, "isp-clockdep");
    if (IS_ERR(dev->clockdep_supply)) {
        pr_err("[%s] Failed : isp-clockdep devm_regulator_get.%pK\n", __func__, dev->clockdep_supply);
        return -EINVAL;
    }
    dev->ispcore_supply = devm_regulator_get(device, "isp-core");
    if (IS_ERR(dev->ispcore_supply)) {
        pr_err("[%s] Failed : isp-core devm_regulator_get.%pK\n", __func__, dev->ispcore_supply);
        return -EINVAL;
    }
    dev->remap_addr = dev->isp_dma;
    set_a7mem_pa(dev->remap_addr);
    set_a7mem_va(dev->isp_dma_va);
    pr_info("[%s] dma_va.%pK\n", __func__, dev->isp_dma_va);

    if ((ret = of_property_read_u32(np, "useisptop", (unsigned int *)(&dev->useisptop))) < 0 ) {
        pr_err("[%s] Failed: useisptop of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] useisptop.0x%x\n", __func__, dev->useisptop);

    if ((ret = of_property_read_u32(np, "clock-num", (unsigned int *)(&dev->clock_num))) < 0 ) {
        pr_err("[%s] Failed: clock-num of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_string_array(np, "clock-names", dev->clk_name, dev->clock_num)) < 0) {
        pr_err("[%s] Failed : clock-names of_property_read_string_array.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_u32_array(np, CLOCKVALUE, dev->ispclk_value, dev->clock_num)) < 0) {
        pr_err("[%s] Failed: clock-value of_property_read_u32_array.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_u32(np, "usedvfs", (unsigned int *)(&dev->usedvfs))) < 0 ) {
        pr_err("[%s] Failed: usedvfs of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] usedvfs.0x%x\n", __func__, dev->usedvfs);

    if ((ret = of_property_read_u32(np, "usepowerdn_clockdn", (unsigned int *)(&dev->usepowerdn_clockdn))) < 0 ) {
        pr_err("[%s] Failed: usepowerdn_clockdn of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] usepowerdn_clockdn.0x%x\n", __func__, dev->usepowerdn_clockdn);

    if ((ret = of_property_read_u32(np, "useclockdep", (unsigned int *)(&dev->useclockdep))) < 0 ) {
        pr_err("[%s] Failed: useclockdep of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] useclockdep.0x%x\n", __func__, dev->useclockdep);

    if ((ret = hisi_isp_cvdr_getdts(np)) != 0) {
        pr_err("[%s] Failed : hisi_isp_cvdr_getdts.%d.\n", __func__, ret);
        return ret;
    }

    if ((ret = hisi_isp_mdc_getdts(np)) != 0) {
        pr_err("[%s] Failed : hisi_isp_cvdr_getdts.%d.\n", __func__, ret);
        return ret;
    }

    if (dev->usepowerdn_clockdn) {
        if ((ret = of_property_read_u32_array(np, "clkdis-dvfs", dev->clkdis_dvfs, dev->clock_num)) < 0) {
            pr_err("[%s] Failed: clkdis-dvfs of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }
        if ((ret = of_property_read_u32_array(np, "clkdis-need-div", dev->clkdis_need_div, dev->clock_num)) < 0) {
            pr_err("[%s] Failed: clkdis-need-div of_property_read_u32_array.%d\n", __func__, ret);
        }
    }

    if ((ret = hisi_isp_nsec_getdts_dvfs(np, dev)) < 0) {
        pr_err("[%s] Failed: hisi_isp_nsec_getdts_dvfs.%d\n", __func__, ret);
        return ret;
    }

    for (i = 0; i < (int)dev->clock_num; i++) {
        dev->ispclk[i] = devm_clk_get(device, dev->clk_name[i]);
        if (IS_ERR_OR_NULL(dev->ispclk[i])) {
            pr_err("[%s] Failed : ispclk.%s.%d.%li\n", __func__, dev->clk_name[i], i, PTR_ERR(dev->ispclk[i]));
            return -EINVAL;
        }
        pr_info("[%s] ISP clock.%d.%s: %d.%d M\n", __func__, i, dev->clk_name[i], dev->ispclk_value[i]/1000000, dev->ispclk_value[i]%1000000);
        if (dev->usepowerdn_clockdn)
            pr_info("[%s] clkdis.%d.%s: %d.%d M\n", __func__, i, dev->clk_name[i], dev->clkdis_dvfs[i]/1000000, dev->clkdis_dvfs[i]%1000000);
    }

    pr_info("[%s] -\n", __func__);

    return 0;
}

static int isp_remap_rsc(struct hisi_isp_nsec *dev)
{
    dev->isp_dma_va = dma_alloc_coherent(dev->device, ISP_MEM_SIZE, &dev->isp_dma, GFP_KERNEL);
    if (unlikely(!dev->isp_dma_va)) {
        pr_err("[%s] isp_dma_va failed\n", __func__);
        return -ENOMEM;
    }
    pr_info("[%s] isp_dma_va.%pK\n", __func__, dev->isp_dma_va);

    return 0;
}

static void isp_unmap_rsc(struct hisi_isp_nsec *dev)
{
    if (dev->isp_dma_va)
        dma_free_coherent(dev->device, ISP_MEM_SIZE, dev->isp_dma_va, dev->isp_dma);

    dev->isp_dma_va = NULL;
}

int hisi_isp_nsec_probe(struct platform_device *pdev)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = &isp_pwr_ops;
    int ret = 0;

    pr_alert("[%s] +\n", __func__);
    ops->refs_a7 = 0;
    ops->refs_isp = 0;
    ops->refs_ispinit = 0;

    dev->device = &pdev->dev;
    dev->isp_pdev = pdev;
    dev->isp_ops = ops;

    if ((ret = isp_remap_rsc(dev)) != 0) {
        pr_err("[%s] failed, isp_remap_src.%d\n", __func__, ret);
        return ret;
    }

    if ((ret = hisi_isp_nsec_getdts(pdev, dev)) != 0) {
        pr_err("[%s] Failed : hisi_isp_nsec_getdts.%d.\n", __func__, ret);
        goto out;
    }

    dev->dvfsmask = (1 << ISPI2C_CLK);

    pr_alert("[%s] -\n", __func__);

    return 0;
out:
    isp_unmap_rsc(dev);

    return ret;
}

int hisi_isp_nsec_remove(struct platform_device *pdev)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    isp_unmap_rsc(dev);
    return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar V150 rproc driver");

