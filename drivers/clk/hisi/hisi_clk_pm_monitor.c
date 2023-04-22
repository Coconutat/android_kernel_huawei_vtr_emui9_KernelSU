#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/clk-private.h>
#endif
#include <linux/clkdev.h>

static HLIST_HEAD(clocks_pm);
static DEFINE_MUTEX(clocks_pm_lock);/*lint !e651 !e708 !e570 !e64 !e785 */

struct pmclk_node {
	const char		*name;
	struct hlist_node		next_node;
};

void pmclk_monitor_enable(void)
{
	struct pmclk_node *pmclk;
	struct clk *clk_node;
	unsigned int ret = 0;

	hlist_for_each_entry(pmclk, &clocks_pm, next_node) {/*lint !e826 !e62 !e64 */
		clk_node = __clk_lookup(pmclk->name);
		if (IS_ERR_OR_NULL(clk_node)) {
			pr_err("%s get failed!\n", __clk_get_name(clk_node));
			return;
		}
		if (__clk_get_enable_count(clk_node) > 0) {
			pr_err("[%s]: cnt=%d !\n",
					__clk_get_name(clk_node), __clk_get_enable_count(clk_node));
			ret++;
		}
	}
	if(0 < ret) {
		WARN_ON(1);/*lint !e730 */
	}
}

static void pmclk_add(struct pmclk_node *clk)
{
	mutex_lock(&clocks_pm_lock);

	hlist_add_head(&clk->next_node, &clocks_pm);

	mutex_unlock(&clocks_pm_lock);
}

static int hisi_pmclk_monitor_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	unsigned int num = 0, i;
	struct pmclk_node *pm_clk = NULL;

	if (!np) {
		pr_err("could not find pmclk node!\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "hisi-pmclk-num", &num);/*lint !e732 */
	if (ret) {
		pr_err("cound not find hisi-pmclk-num property!\n");
		return -EINVAL;
	}
	for (i = 0; i < num; i++) {
		pm_clk = kzalloc(sizeof(struct pmclk_node), GFP_KERNEL);/*lint !e429 */
		if(!pm_clk) {
			pr_err("[%s] fail to alloc pm_clk!\n", __func__);
			goto out;
		}

		ret = of_property_read_string_index(np, "clock-names", i, &(pm_clk->name));
		if(0 != ret) {
			pr_err("%s:Failed to get clk-names\n", __func__);
			kfree(pm_clk);
			pm_clk = NULL;
			goto out;
		}

		pmclk_add(pm_clk);
		pm_clk = NULL;

	}
out:
	pr_err("pm clk monitor setup!\n");

	return 0;/*lint !e429 */
}

static int hisi_pmclk_monitor_remove(struct platform_device *pdev)
{
	return 0;
}/*lint !e715 */

static struct of_device_id hisi_pmclk_of_match[] = {
	{ .compatible = "hisilicon,pm-clk-monitor" },/*lint !e785 */
	{ },/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, hisi_pmclk_of_match);

static struct platform_driver hisi_pmclk_monitor_driver = {
	.probe          = hisi_pmclk_monitor_probe,
	.remove         = hisi_pmclk_monitor_remove,
	.driver         = {
		.name   = "hisi_pmclk",
		.owner  = THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(hisi_pmclk_of_match),
	},/*lint !e785 */
};/*lint !e785 */

static int __init hisi_pmclk_monitor_init(void)
{
	return platform_driver_register(&hisi_pmclk_monitor_driver);/*lint !e64 */
}
/*lint -e528 -esym(528,*) */
fs_initcall(hisi_pmclk_monitor_init);
/*lint -e528 +esym(528,*) */