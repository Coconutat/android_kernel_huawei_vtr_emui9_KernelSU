#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/amba/bus.h>
#include <linux/pm_runtime.h>
#include <linux/coresight.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/clk.h>

#include "cfc_coresight.h"

#define TOPCS_ATCLK_NAME	"clk_topcs_atclk"

DEFINE_SPINLOCK(cfc_coresight_spinlock);

struct cfc_coresight_drvdata;

struct cfc_coresight_ops {
	void (*prepare)(struct cfc_coresight_drvdata *drvdata, int port);
	void (*flush)(struct cfc_coresight_drvdata *drvdata);
};

struct cfc_coresight_drvdata {
	struct device *dev;
	struct coresight_platform_data *pdata;
	struct clk *atclk;
	struct cfc_coresight_ops *ops;
	struct cfc_coresight_drvdata *next_dev;
	int used_in_tee;
	atomic_t clk_prepared;
	void __iomem *base;
};
static struct cfc_coresight_drvdata *etmoncore[NR_CPUS];

static void cfc_enable_device(struct cfc_coresight_drvdata *drvdata, int port)
{
	int child_port = 0;
	if (!drvdata)
		return;

	// Recursively pass the remote endpoint port number to the next device
	if (drvdata->pdata->child_ports != NULL)
		child_port = drvdata->pdata->child_ports[0];

	cfc_enable_device(drvdata->next_dev, child_port);
	if (drvdata->ops && drvdata->ops->prepare)
		drvdata->ops->prepare(drvdata, port);
}

static void cfc_disable_device(struct cfc_coresight_drvdata *drvdata)
{
	if (!drvdata)
		return;

	if (drvdata->ops && drvdata->ops->flush)
		drvdata->ops->flush(drvdata);
	cfc_disable_device(drvdata->next_dev);
}

static inline void cfc_coresight_work(bool enable)
{
	int cpu = raw_smp_processor_id();
	struct cfc_coresight_drvdata *drvdata = etmoncore[cpu];

	if (!drvdata)
		return;
	if (enable)
		cfc_enable_device(drvdata, 0);
	else
		cfc_disable_device(drvdata);
}

void cfc_enable_coresight(void)
{
	cfc_coresight_work(true);
}

void cfc_disable_coresight(void)
{
	cfc_coresight_work(false);
}

#define CORESIGHT_LAR		0xfb0
static inline void CS_UNLOCK(void __iomem *addr)
{
	writel_relaxed(CORESIGHT_UNLOCK, addr + CORESIGHT_LAR);
	/* Make sure everyone has seen this */
	mb();
}

static inline void CS_LOCK(void __iomem *addr)
{
	/* Wait for things to settle */
	mb();
	writel_relaxed(0x0, addr + CORESIGHT_LAR);
}

#define TIMEOUT_LOOPS	1800000
static int cfc_coresight_timeout(void __iomem *addr, u32 offset,
				 int position, int value)
{
	int i;
	u32 val;

	for (i = TIMEOUT_LOOPS; i > 0; i--) {
		val = __raw_readl(addr + offset);
		/* waiting on the bit to go from 0 to 1 */
		if (value) {
			if (val & BIT(position))
				return 0;
			/* waiting on the bit to go from 1 to 0 */
		} else {
			if (!(val & BIT(position)))
				return 0;
		}

		/*
		 * Delay is arbitrary - the specification doesn't say how long
		 * we are expected to wait.  Extra check required to make sure
		 * we don't wait needlessly on the last iteration.
		 */
	}

	return -EAGAIN;
}

/* Funnel driver */
#define FUNNEL_FUNCTL		0x000
#define FUNNEL_PRICTL		0x004

#define FUNNEL_HOLDTIME_MASK	0xf00
#define FUNNEL_HOLDTIME_SHFT	0x8
#define FUNNEL_HOLDTIME		(0x7 << FUNNEL_HOLDTIME_SHFT)

#define NR_MAX_FUNNEL_PORT	8
static void
cfc_coresight_funnel_prepare(struct cfc_coresight_drvdata *drvdata, int port)
{
	u32 functl;

	if (port >= NR_MAX_FUNNEL_PORT)
		return;

	CS_UNLOCK(drvdata->base);
	functl = readl_relaxed(drvdata->base + FUNNEL_FUNCTL);
	functl &= ~FUNNEL_HOLDTIME_MASK;
	functl |= FUNNEL_HOLDTIME;
	// Only open the port that is actually being used
	functl |= 1 << port;
	writel_relaxed(functl, drvdata->base + FUNNEL_FUNCTL);
	CS_LOCK(drvdata->base);
}

static struct cfc_coresight_ops funnel_ops = {
	.prepare = cfc_coresight_funnel_prepare,
	.flush = NULL,
};

/* ETF driver */

#define TMC_STS			0x00c
#define TMC_STS_TMCREADY_BIT	2

#define TMC_CTL			0x020
#define TMC_CTL_CAPT_EN		BIT(0)

#define TMC_MODE		0x028
#define TMC_MODE_HARDWARE_FIFO	2

#define TMC_BUFWM		0x034

#define TMC_FFCR		0x304
#define TMC_FFCR_FLUSHMAN_BIT	6
#define TMC_FFCR_EN_FMT		BIT(0)
#define TMC_FFCR_EN_TI		BIT(1)
#define TMC_FFCR_FLUSHMAN	BIT(TMC_FFCR_FLUSHMAN_BIT)
#define TMC_FFCR_STOP_ON_FLUSH	BIT(12)

static void
cfc_coresight_etf_prepare(struct cfc_coresight_drvdata *drvdata, int port)
{
	CS_UNLOCK(drvdata->base);

	writel_relaxed(TMC_MODE_HARDWARE_FIFO, drvdata->base + TMC_MODE);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI,
		       drvdata->base + TMC_FFCR);
	writel_relaxed(0x0, drvdata->base + TMC_BUFWM);
	writel_relaxed(TMC_CTL_CAPT_EN, drvdata->base + TMC_CTL);

	CS_LOCK(drvdata->base);
}

static void
cfc_coresight_etf_flush(struct cfc_coresight_drvdata *drvdata)
{
	u32 ffcr;

	CS_UNLOCK(drvdata->base);
	ffcr = readl_relaxed(drvdata->base + TMC_FFCR);
	ffcr |= TMC_FFCR_STOP_ON_FLUSH;
	writel_relaxed(ffcr, drvdata->base + TMC_FFCR);
	ffcr |= TMC_FFCR_FLUSHMAN;
	writel_relaxed(ffcr, drvdata->base + TMC_FFCR);

	cfc_coresight_timeout(drvdata->base, TMC_FFCR, TMC_FFCR_FLUSHMAN_BIT, 0);
	cfc_coresight_timeout(drvdata->base, TMC_STS, TMC_STS_TMCREADY_BIT, 1);

	writel_relaxed(0x0, drvdata->base + TMC_CTL);
	CS_LOCK(drvdata->base);
}

static struct cfc_coresight_ops etf_ops = {
	.prepare = cfc_coresight_etf_prepare,
	.flush = cfc_coresight_etf_flush,
};

/* ETM driver */
#define TRCPRGCTLR			0x004
#define TRCSTATR			0x00C
#define TRCSTATR_IDLE_BIT		0

#define TRCBBCTLR			0x03C
#define TRCTRACEIDR			0x040
#define TRCCONFIGR			0x010
#define TRCEVENTCTL0R			0x020
#define TRCEVENTCTL1R			0x024
#define TRCSTALLCTLR			0x02C
#define TRCTSCTLR			0x030
#define TRCSYNCPR			0x034
#define TRCVICTLR			0x080
#define TRCVIIECTLR			0x084
#define TRCVISSCTLR			0x088
#define TRCOSLAR			0x300
#define TRCACVRn(n)			(0x400 + (n * 8))
#define TRCACATRn(n)			(0x480 + (n * 8))

extern int __cfc_audit_start[];
extern int __cfc_audit_stop[];

static void
cfc_coresight_etm_prepare(struct cfc_coresight_drvdata *drvdata, int port)
{
	unsigned long trcstatr;

	CS_UNLOCK(drvdata->base);
	/* os unlock */
	writel_relaxed(0x0, drvdata->base + TRCOSLAR);
	isb();

	trcstatr = readl_relaxed(drvdata->base + TRCSTATR);

	if (!(trcstatr & BIT(TRCSTATR_IDLE_BIT))) {
		/* Disable the trace unit before programming trace registers */
		writel_relaxed(0, drvdata->base + TRCPRGCTLR);

		/* wait for TRCSTATR.IDLE to go up */
		cfc_coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 1);
		trcstatr = readl_relaxed(drvdata->base + TRCSTATR);
	}

	if (!(trcstatr & BIT(TRCSTATR_IDLE_BIT))) {
		dev_err(drvdata->dev, "ETM not ready\n");
		goto out;
	}

	writel_relaxed( BIT(0) | /* RES1 */
			BIT(3) | /* BB */
			BIT(6) | /* CID (context id) */
			0, drvdata->base + TRCCONFIGR);
	writel_relaxed(0x0, drvdata->base + TRCEVENTCTL0R);
	writel_relaxed(0x0, drvdata->base + TRCEVENTCTL1R);
	writel_relaxed(0x0, drvdata->base + TRCSTALLCTLR);
	writel_relaxed(0x0, drvdata->base + TRCSYNCPR);
	writel_relaxed(drvdata->pdata->cpu + 1, drvdata->base + TRCTRACEIDR);
	writel_relaxed(0, drvdata->base + TRCTSCTLR);
	writel_relaxed(BIT(20) | /*BIT(21)*/ 0 | BIT(22) |       0  |	/* Trace NS EL1 only */
		       BIT(16) |   BIT(17)     |       0 | BIT(19)  |	/* Disable all S tracing */
		       BIT(0)  | /* Trace everything whti start-stop logic */
		       BIT(9) /* SSStatus: start-stop logic start at 'started' */
		       , drvdata->base + TRCVICTLR);

	/* Address range comparator */
	writeq_relaxed((uintptr_t)(&__cfc_audit_start), drvdata->base + TRCACVRn(0));
	writeq_relaxed(0x0ULL, 		drvdata->base + TRCACATRn(0));
	writeq_relaxed((uintptr_t)(&__cfc_audit_stop) - 1, drvdata->base + TRCACVRn(1));
	writeq_relaxed(0x0ULL, 		drvdata->base + TRCACATRn(1));

	/* Branch boardcasting */
	writel_relaxed(BIT(0) | /* Use address range comparator 0 */
		       BIT(8)   /* Include mode */
		       , drvdata->base + TRCBBCTLR);

	/* Select address range comparator pair 0 as include */
	writel_relaxed(BIT(0), drvdata->base + TRCVIIECTLR);
	/* Clean start stop comparator */
	writel_relaxed(0x0, drvdata->base + TRCVISSCTLR);
	writel_relaxed(1, drvdata->base + TRCPRGCTLR);
	cfc_coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 0);
out:
	CS_LOCK(drvdata->base);
}

static void
cfc_coresight_etm_flush(struct cfc_coresight_drvdata *drvdata)
{
	u32 control;

	CS_UNLOCK(drvdata->base);
	/* os unlock */
	writel_relaxed(0x0, drvdata->base + TRCOSLAR);
	isb();

	control = readl_relaxed(drvdata->base + TRCPRGCTLR);
	/* EN, bit[0] Trace unit enable bit */
	control &= ~0x1;

	/* make sure everything completes before disabling */
	mb();
	isb();
	writel_relaxed(control, drvdata->base + TRCPRGCTLR);
	cfc_coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 1);
	CS_LOCK(drvdata->base);
}

static struct cfc_coresight_ops etm_ops = {
	.prepare = cfc_coresight_etm_prepare,
	.flush = cfc_coresight_etm_flush,
};

#define MAX_CORESIGHT_DRVDATA	16
static unsigned int nr_cfc_coresight_drvdata;
static struct cfc_coresight_drvdata all_drvdata[MAX_CORESIGHT_DRVDATA];
static struct cfc_coresight_drvdata *
alloc_cfc_coresight_drvdata(struct amba_device *adev)
{
	if (nr_cfc_coresight_drvdata >= MAX_CORESIGHT_DRVDATA) {
		dev_err(&adev->dev, "CFC_CS: too many coresight devices (%d)\n",
			nr_cfc_coresight_drvdata);
		return NULL;
	}
	return &all_drvdata[nr_cfc_coresight_drvdata++];
}

static void
cfc_coresight_connect(struct cfc_coresight_drvdata *prev, struct cfc_coresight_drvdata *next)
{
	if (prev->next_dev) {
		dev_err(prev->dev, "CFC_CS: multiple outputs: %s and %s\n",
			prev->next_dev->pdata->name, next->pdata->name);
		return;
	}

	prev->next_dev = next;
}

static void
cfc_coresight_fixup_conn(struct cfc_coresight_drvdata *drvdata)
{
	unsigned int i;

	/* See if drvdata is someone's child */
	for (i = 0; i < nr_cfc_coresight_drvdata; i++) {
		struct cfc_coresight_drvdata *d = &all_drvdata[i];

		if (d == drvdata || !d->pdata)
			continue;

		if (strncmp(drvdata->pdata->name,
			   d->pdata->child_names[0],
			   strlen(drvdata->pdata->name)) != 0)
			continue;

		cfc_coresight_connect(d, drvdata);
	}

	/* See if drvdata is someone's parent */
	for (i = 0; i < nr_cfc_coresight_drvdata; i++) {
		struct cfc_coresight_drvdata *d = &all_drvdata[i];

		if (strncmp(drvdata->pdata->child_names[0],
			   d->pdata->name,
			   strlen(drvdata->pdata->child_names[0])) != 0)
			continue;
		cfc_coresight_connect(drvdata, d);
	}
}

static struct cfc_coresight_drvdata *
cfc_coresight_probe(struct amba_device *adev)
{
	struct device *dev = &adev->dev;
	struct resource *res = &adev->res;
	struct cfc_coresight_drvdata *drvdata;
	struct coresight_platform_data *pdata;
	struct device_node *np = adev->dev.of_node;
	void *__iomem base;

	if (!np) {
		dev_err(dev, "CFC_CS: No np\n");
		return NULL;
	}

	if (of_property_read_bool(np, "disable-in-cfc")) {
		dev_info(dev, "CFC_CS: device disabled in cfc\n");
		return NULL;
	}

	pdata = of_get_coresight_platform_data(dev, np);
	if (!pdata) {
		dev_err(dev, "CFC_CS: No pdata\n");
		return NULL;
	}

	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base)) {
		dev_err(dev, "CFC_CS: No base address\n");
		return NULL;
	}

	drvdata = alloc_cfc_coresight_drvdata(adev);
	if (!drvdata)
		return NULL;

	drvdata->used_in_tee = 0;
	if (of_property_read_bool(np, "cfc-used-in-tee")) {
		drvdata->ops = NULL;
		drvdata->used_in_tee = 1;
	}

	atomic_set(&drvdata->clk_prepared, 0);
	drvdata->pdata = pdata;
	drvdata->base = base;
	drvdata->dev = dev;

	cfc_coresight_fixup_conn(drvdata);

	return drvdata;
}

static int cfc_etm4_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct cfc_coresight_drvdata *drvdata;
	struct device *dev = &adev->dev;
	int cpu;

	dev_info(dev, "CFC_CS: probing etm at %lx\n", (unsigned long)adev->res.start);

	drvdata = cfc_coresight_probe(adev);
	if (!drvdata || !drvdata->pdata)
		return -EINVAL;

	if (!drvdata->used_in_tee)
		drvdata->ops = &etm_ops;

	cpu = drvdata->pdata->cpu;
	if (cpu >= NR_CPUS) {
		dev_err(dev, "CFC_CS: invalid CPU in dt: %d\n", cpu);
		return -EINVAL;
	}
	if (etmoncore[cpu]) {
		dev_err(dev, "CFC_CS: multiple ETM on CPU %d\n", cpu);
		return -EINVAL;
	}
	etmoncore[cpu] = drvdata;

	return 0;
}

static int cfc_funnel_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct cfc_coresight_drvdata *drvdata = cfc_coresight_probe(adev);
	struct device *dev = &adev->dev;

	dev_info(dev, "CFC_CS: probing funnel at %lx\n", (unsigned long)adev->res.start);

	if (!drvdata)
		return -EINVAL;

	drvdata->atclk = devm_clk_get(&adev->dev, TOPCS_ATCLK_NAME);	/* optional */
	if (!IS_ERR_OR_NULL(drvdata->atclk)) {
		dev_info(dev, "CFC_CS: current clk enb cnt %d\n",
				clk_get_enable_count(drvdata->atclk));
	}

	if (!drvdata->used_in_tee)
		drvdata->ops = &funnel_ops;

	return 0;
}

static int cfc_tmc_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct cfc_coresight_drvdata *drvdata;
	struct device *dev = &adev->dev;

	dev_info(dev, "CFC_CS: probing tmc at %lx\n", (unsigned long)adev->res.start);

	drvdata = cfc_coresight_probe(adev);
	if (!drvdata)
		return -EINVAL;

	drvdata->atclk = devm_clk_get(&adev->dev, TOPCS_ATCLK_NAME);	/* optional */
	if (!IS_ERR_OR_NULL(drvdata->atclk)) {
		dev_info(dev, "CFC_CS: current clk enb cnt %d\n",
				clk_get_enable_count(drvdata->atclk));
	}

	if (!drvdata->used_in_tee)
		drvdata->ops = &etf_ops;

	return 0;
}

void cfc_prepare_clk_pm(void)
{
	unsigned int i;

	for (i = 0; i < nr_cfc_coresight_drvdata; i++) {
		if (!IS_ERR_OR_NULL(all_drvdata[i].atclk)) {
			if (clk_prepare_enable(all_drvdata[i].atclk) == 0)
				atomic_inc(&all_drvdata[i].clk_prepared);
		}
	}
}

void cfc_unprepare_pm_clk(void)
{
	unsigned int i;

	for (i = 0; i < nr_cfc_coresight_drvdata; i++) {
		if (!IS_ERR_OR_NULL(all_drvdata[i].atclk) &&
				atomic_read(&all_drvdata[i].clk_prepared) > 0) {
			clk_disable_unprepare(all_drvdata[i].atclk);
			atomic_dec(&all_drvdata[i].clk_prepared);
		}
	}
}

static struct amba_id cfc_etm4_ids[] = {
	{			/* ETM 4.0 - Qualcomm */
	 .id = 0x0003b95d,
	 .mask = 0x0003ffff,
	 .data = "ETM 4.0",
	 },
	{			/* ETM 4.0 - Juno board */
	 .id = 0x000bb95e,
	 .mask = 0x000fffff,
	 .data = "ETM 4.0",
	 },
	{			/* ETM 4.0 - A72 board */
	 .id = 0x000bb95a,
	 .mask = 0x000fffff,
	 .data = "ETM 4.0",
	 },
	{			/* ETM 4.0 - Atermis board */
	 .id = 0x000bb959,
	 .mask = 0x000fffff,
	 .data = "ETM 4.0",
	 },
	{			/* ETM 4.0 - Ananke */
		.id	= 0x000bbd05,
		.mask	= 0x000fffff,
		.data	= "Cortex-A55 ETM 4.0",
	},
	{			/* ETM 4.0 - Enyo */
		.id	= 0x000bbd0b,
		.mask	= 0x000fffff,
		.data	= "Cortex-Enyo ETM 4.0",
	},
	{0, 0},
};
static struct amba_driver cfc_etm4x_driver = {
	.drv = {
		.name = "coresight-cfc_etm4x",
		},
	.probe = cfc_etm4_probe,
	.id_table = cfc_etm4_ids,
};
module_amba_driver(cfc_etm4x_driver);

static struct amba_id cfc_funnel_ids[] = {
	{
	 .id = 0x0003b908,
	 .mask = 0x0003ffff,
	 },
	{0, 0},
};
static struct amba_driver cfc_funnel_driver = {
	.drv = {
		.name = "coresight-funnel",
		.owner = THIS_MODULE,
		},
	.probe = cfc_funnel_probe,
	.id_table = cfc_funnel_ids,
};
module_amba_driver(cfc_funnel_driver);

static struct amba_id cfc_tmc_ids[] = {
	{
	 .id = 0x0003b961,
	 .mask = 0x0003ffff,
	 },
	{0, 0},
};
static struct amba_driver cfc_tmc_driver = {
	.drv = {
		.name = "coresight-tmc",
		.owner = THIS_MODULE,
		},
	.probe = cfc_tmc_probe,
	.id_table = cfc_tmc_ids,
};
module_amba_driver(cfc_tmc_driver);
