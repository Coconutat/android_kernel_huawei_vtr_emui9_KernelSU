/*
 *Device driver for vbat drop protect
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <hisi_vbat_drop_protect.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>

#ifdef CONFIG_HISI_HW_VOTE
#include <linux/hisi/hisi_hw_vote.h>

static struct hvdev *limit_bigfreq_hvdev    = NULL;
static struct hvdev *limit_middlefreq_hvdev = NULL;
static struct hvdev *limit_gpufreq_hvdev    = NULL;
#endif

#ifndef STATIC
#define STATIC static
#endif


#ifdef VBAT_DROP_TEST

/*1:auto 2 div*/
static void hisi_vbat_drop_print_auto_div_state(struct hisi_vbat_drop_protect_dev *di)
{
    if (!di || !di->pmctrl_base)
        return;
    /*0x41c,0x3fc,0x3dc,0x43c,0x45c*/
    pr_err("[%s]vbat_drop:L = 0x%x,M = 0x%x,B = 0x%x,L3 = 0x%x, G = 0x%x!\n", __func__,
                         (unsigned int)(readl(LITTLE_VOL_DROP_EN_STAT_ADDR(di->pmctrl_base)) & LITTLE_VOL_DROP_EN_STAT_BIT),
                         (unsigned int)(readl(MIDDLE_VOL_DROP_EN_STAT_ADDR(di->pmctrl_base)) & MIDDLE_VOL_DROP_EN_STAT_BIT),
                         (unsigned int)(readl(BIG_VOL_DROP_EN_STAT_ADDR(di->pmctrl_base))    & BIG_VOL_DROP_EN_STAT_BIT),
                         (unsigned int)(readl(L3_VOL_DROP_EN_STAT_ADDR(di->pmctrl_base))     & L3_VOL_DROP_EN_STAT_BIT),
                         (unsigned int)(readl(GPU_VOL_DROP_EN_STAT_ADDR(di->pmctrl_base))    & GPU_VOL_DROP_EN_STAT_BIT));
}
#endif

#ifdef CONFIG_HISI_HW_VOTE
/********************************************************************
  Function:        hisi_cluster_freq_limit_init
  Description:     freq limt init
  Input:           struct device *dev   ---- vbat drop protect device
  Output:          NULL
  Return:          NULL
  Remart:          get freq core from dts and register to hvdev
**********************************************************************/
static void hisi_vbat_drop_cluster_freq_limit_init(struct device *dev)
{
	struct device_node *np;
	const char *ch_name = NULL;
	const char *vsrc = NULL;
	int ret;

    if (!dev)
        return;

    np = dev->of_node;

    /*get big cpu fre from dts and register to hvdev*/
	ret = of_property_read_string_index(np, "bigfreq-limit-channel", 0, &ch_name);
	if (ret) {
		dev_err(dev, "[%s]:parse big cpu channel name fail,not channel!\n", __func__);
		goto middle_limit_init;
	}

	ret = of_property_read_string_index(np, "bigfreq-limit-channel", 1, &vsrc);
	if (ret) {
		dev_err(dev, "[%s]:parse big cpu  vote src fail!\n", __func__);
		goto middle_limit_init;
	}
	limit_bigfreq_hvdev = hisi_hvdev_register(dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(limit_bigfreq_hvdev)) {
		dev_err(dev, "[%s]: bigfreq limit vote register fail!\n", __func__);
	}

    /*get middle cpu fre from dts and register to hvdev*/
middle_limit_init:
	ret = of_property_read_string_index(np, "middlefreq-limit-channel", 0, &ch_name);
	if (ret) {
		dev_err(dev, "[%s]:parse middle cpu channel name fail,not channel!\n", __func__);
		goto gpu_limit_init;
	}

	ret = of_property_read_string_index(np, "middlefreq-limit-channel", 1, &vsrc);
	if (ret) {
		dev_err(dev, "[%s]:parse middle cpu vote src fail!\n", __func__);
		goto gpu_limit_init;
	}
	limit_middlefreq_hvdev = hisi_hvdev_register(dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(limit_middlefreq_hvdev)) {
		dev_err(dev, "[%s]: middlefreq limit vote register fail!\n", __func__);
	}
     /*get gpu fre from dts and register to hvdev*/
gpu_limit_init:
	ret = of_property_read_string_index(np, "gpufreq-limit-channel", 0, &ch_name);
	if (ret) {
		dev_err(dev, "[%s]:parse gpu channel name fail, not channel!\n", __func__);
		goto out;
	}

	ret = of_property_read_string_index(np, "gpufreq-limit-channel", 1, &vsrc);
	if (ret) {
		dev_err(dev, "[%s]:parse gpu vote src fail!\n", __func__);
		goto out;
	}

	limit_gpufreq_hvdev = hisi_hvdev_register(dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(limit_gpufreq_hvdev)) {
		dev_err(dev, "[%s]: gpufreq limit vote register fail!\n", __func__);
	}

out:
	return;
}


/********************************************************************
  Function:        hisi_vbat_drop_cluster_freq_set
  Description:     set core freq
  Input:           enum vbat_drop_freq  freq_type ---- core freq.
  Output:          NULL
  Return:          NULL
  Remart:          set core freq to min or normal.
**********************************************************************/

STATIC void hisi_vbat_drop_cluster_freq_set(enum vbat_drop_freq freq_type)
{
    int ret;

    /*vote big cpu middle cpu and gpu to lowest freq*/
    pr_err("[%s]freq type [%d]\n", __func__, freq_type);

    if (MIN_FREQ == freq_type) {
        ret = hisi_hv_set_freq(limit_bigfreq_hvdev, 0);
        if (!ret)
            pr_err("[%s]big cluster votes to lowest frequency\n", __func__);

        ret = hisi_hv_set_freq(limit_middlefreq_hvdev, 0);
        if (!ret)
            pr_err("[%s]middle cluster votes to lowest frequency\n", __func__);

        ret = hisi_hv_set_freq(limit_gpufreq_hvdev, 0);
        if (!ret)
            pr_err("[%s]gpu votes to lowest frequency\n", __func__);

    } else if (RESTOR_FREQ == freq_type) {
      /*vote big cpu middle cpu and gpu to restore normal freq*/
		ret = hisi_hv_set_freq(limit_bigfreq_hvdev, 0xFFFFFFFF);
		if (!ret)
			pr_err("[%s]big cluster returns to normal frequency\n",__func__);

		ret = hisi_hv_set_freq(limit_middlefreq_hvdev, 0xFFFFFFFF);
		if (!ret)
			pr_err("[%s]middle cluster returns to normal frequency\n", __func__);

		ret = hisi_hv_set_freq(limit_gpufreq_hvdev, 0xFFFFFFFF);
		if (!ret)
			pr_err("[%s]gpu returns to normal frequency\n", __func__);

    } else
        ;
}

#endif

/********************************************************************
  Function:        hisi_vbat_drop_interrupt_work
  Description:     check battery vol and restore normal freq .
  Input:           work: vbat dorp interrupt workqueue.
  Output:          NULL
  Return:          NULL
  Remart:          NA
**********************************************************************/
static void hisi_vbat_drop_interrupt_work(struct work_struct *work)
{
	struct hisi_vbat_drop_protect_dev *di =
        container_of(work, struct hisi_vbat_drop_protect_dev, vbat_drop_irq_work.work);
    static int battery_check_cnt  = 0;
    static int bat_vol_normal_cnt = 0;
    int vbatt_mv;

    battery_check_cnt++;
	vbatt_mv = hisi_battery_voltage();
	pr_err("[%s]battery_check_cnt:[%d],battery voltage = %d mv\n", __func__, battery_check_cnt, vbatt_mv);

    /*if vbat vol is more than exit vol, work will exit after trying VBAT_DROP_VOL_NORMAL_CNT timers success */
	if (vbatt_mv > (int)di->vbat_drop_vol_mv + VBAT_DROP_VOL_INC_MV) {

		pr_err("[%s] bat_vol_normal_cnt = %d!\n", __func__, bat_vol_normal_cnt);

        bat_vol_normal_cnt++;

        /* Try a few times*/
        if (bat_vol_normal_cnt > VBAT_DROP_VOL_NORMAL_CNT ) {
        #ifdef CONFIG_HISI_HW_VOTE
            hisi_vbat_drop_cluster_freq_set(RESTOR_FREQ);
        #endif
		    /*clear interrupt status and cancel auto 2 div*/
            HISI_VBAT_DROP_PMIC_REG_WRITE(PMIC_VSYS_DROP_IRQ_REG, PMIC_VSYS_DROP_IRQ_CLEAR);
		    enable_irq(di->vbat_drop_irq);
            battery_check_cnt = 0;
            bat_vol_normal_cnt = 0;
            pr_err("[%s] exit work,vbat vol restore!\n", __func__);
        #ifdef VBAT_DROP_TEST
            hisi_vbat_drop_print_auto_div_state(di);
        #endif
		    wake_unlock(&di->vbatt_check_lock);/*lint !e455*/
            return ;
        } else
            queue_delayed_work(system_power_efficient_wq,&di->vbat_drop_irq_work, msecs_to_jiffies(2000));

	} else {
	    bat_vol_normal_cnt = 0;
		queue_delayed_work(system_power_efficient_wq, &di->vbat_drop_irq_work, msecs_to_jiffies(2000));
	}
}
/********************************************************************
  Function:        hisi_vbat_drop_irq_handler
  Description:     handle vbat drop irq .
  Input:           irq: vbat drop irq, *data: hisi_vbat_drop_protect_dev
  Output:          NULL
  Return:          IRQ_HANDLED:success , others:fail
  Remart:          set core min freq and schedule vat drop check work.
**********************************************************************/
static irqreturn_t hisi_vbat_drop_irq_handler(int irq, void *data)
{
	struct hisi_vbat_drop_protect_dev *di = (struct hisi_vbat_drop_protect_dev *)data;

	wake_lock(&di->vbatt_check_lock);

    pr_err("[%s] enter vbat handle!\n", __func__);

	/*interrupt mask*/
	disable_irq_nosync(di->vbat_drop_irq);
    /*printf auto div register state*/
#ifdef VBAT_DROP_TEST
    hisi_vbat_drop_print_auto_div_state(di);
#endif
#ifdef CONFIG_HISI_HW_VOTE
    hisi_vbat_drop_cluster_freq_set(MIN_FREQ);
#endif
	/*delayed work: check battery voltage*/
	queue_delayed_work(system_power_efficient_wq, &di->vbat_drop_irq_work, msecs_to_jiffies(0));

	return IRQ_HANDLED;  /*lint !e454*/ /*work unlock*/
}

/**********************************************************************************************
  Function:        hisi_vbat_drop_parse_dts
  Description:     parse dts for vbat_drop protect function.
  Input:           struct hisi_vbat_drop_protect_dev *di,   ---- vbat drop protect device
  Output:          NULL
  Return:          0:success,others:fail.
  Remart:          NA
***************************************************************************************************/
static int hisi_vbat_drop_parse_dts(struct hisi_vbat_drop_protect_dev *di)
{
    struct device_node* np;
    struct device_node *base_np;

    if(!di)
        return -1;

    np = di->dev->of_node;
    if(!np)
        return -1;

    /*get vbat drop vol mv*/
    if (of_property_read_u32(np, "vbat_drop_vol_mv",&di->vbat_drop_vol_mv)){
	    di->vbat_drop_vol_mv = VBAT_DROP_VOL_DEFAULT;
        dev_err(di->dev, "[%s]: get vbat drop fail!\n", __func__);
    }
    dev_err(di->dev, "[%s]: vbat_drop_vol_mv = [%d]mv!\n", __func__, di->vbat_drop_vol_mv);

    /*get soc base*/
    base_np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
    if (!base_np) {
        dev_err(di->dev, "[%s] pmctrl node not found!\n", __func__);
        return -1;
    }
    di->pmctrl_base = of_iomap(base_np, 0);
    if (!di->pmctrl_base) {
        dev_err(di->dev, "[%s] pmctrl_base of iomap fail!\n", __func__);
        return -1;
    }

    /*get auto div flag */
    if (of_property_read_u32(np, "big_cpu_auto_div_en",&di->big_cpu_auto_div_en)){
	    di->big_cpu_auto_div_en = 0;
        dev_err(di->dev, "[%s]: get big_cpu_auto_div_en fail!\n", __func__);
    }
    if (of_property_read_u32(np, "middle_cpu_auto_div_en",&di->middle_cpu_auto_div_en)){
	    di->middle_cpu_auto_div_en = 0;
        dev_err(di->dev, "[%s]: get middle_cpu_auto_div_en fail!\n", __func__);
    }
    if (of_property_read_u32(np, "little_cpu_auto_div_en",&di->little_cpu_auto_div_en)){
	    di->little_cpu_auto_div_en = 0;
        dev_err(di->dev, "[%s]: get little_cpu_auto_div_en fail!\n", __func__);
    }
    if (of_property_read_u32(np, "l3_cpu_auto_div_en",&di->l3_auto_div_en)){
	    di->l3_auto_div_en = 0;
        dev_err(di->dev, "[%s]: get l3_auto_div_en fail!\n", __func__);
    }
    if (of_property_read_u32(np, "gpu_auto_div_en",&di->gpu_auto_div_en)){
	    di->gpu_auto_div_en = 0;
        dev_err(di->dev, "[%s]: get gpu_auto_div_en fail!\n", __func__);
    }

    return 0;
}
/*******************************************************
  Function:        hisi_vbat_drop_cpu_drop_en
  Description:     enable or disable Automatic 1/2 Frequency Division(cpu/gpu/l3)
  Input:           struct hisi_vbat_drop_protect_dev *di,   ---- vbat drop protect device
                   enum drop_freq_en core  :div core type
                   int en: 1:enable,0:disable
  Output:          NULL
  Return:          NULL
  Remart:          pmuctrl registers for cpu div control can't Simultaneous been writted by lpm3
********************************************************/
static void  hisi_vbat_drop_cpu_drop_en(struct hisi_vbat_drop_protect_dev *di, enum drop_freq_en core, int en)
{
    void __iomem *base;
    u32 reg_value;

    if (!di) {
        pr_err("%s:di is NULL\n", __func__);
        return;
    }

    base = di->pmctrl_base;
    if (!base) {
        pr_err("%s:base is NULL\n", __func__);
        return;
    }

    if (!en) {
        reg_value = readl(BIG_VOL_DROP_EN_ADDR(base));
        writel(reg_value & ~BIG_VOL_DROP_EN_BIT, BIG_VOL_DROP_EN_ADDR(base));/*CUP_BIG  0x3c8*/

        reg_value = readl(MIDDLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value & ~MIDDLE_VOL_DROP_EN_BIT, MIDDLE_VOL_DROP_EN_ADDR(base));/*CUP_MIDDLE 0x3e8*/

        reg_value = readl(LITTLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value& ~LITTLE_VOL_DROP_EN_BIT, LITTLE_VOL_DROP_EN_ADDR(base));/*CUP_LITTLE 0x408*/

        reg_value = readl(L3_VOL_DROP_EN_ADDR(base));
        writel(reg_value & ~L3_VOL_DROP_EN_BIT, L3_VOL_DROP_EN_ADDR(base));/*CUP_L3 0x428*/

        reg_value = readl(GPU_VOL_DROP_EN_ADDR(base));
        writel(reg_value & ~GPU_VOL_DROP_EN_BIT, GPU_VOL_DROP_EN_ADDR(base));/*CUP_GPU 0x448*/

        pr_err("[%s]close cpu all!\n", __func__);
        return;

    }
    switch (core) {
    case BIG_CPU:
        reg_value = readl(BIG_VOL_DROP_EN_ADDR(base));
        writel(reg_value|BIG_VOL_DROP_EN_BIT, BIG_VOL_DROP_EN_ADDR(base));
        pr_err("[%s]enable cpu big!\n", __func__);
        break;
    case MIDDLE_CPU:
        reg_value = readl(MIDDLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value|MIDDLE_VOL_DROP_EN_BIT, MIDDLE_VOL_DROP_EN_ADDR(base));
        pr_err("[%s]enable cpu middle!\n", __func__);
        break;

    case LITTLE_CPU:
        reg_value = readl(LITTLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value|LITTLE_VOL_DROP_EN_BIT, LITTLE_VOL_DROP_EN_ADDR(base));
        pr_err("[%s]enable cpu little!\n", __func__);
        break;

    case L3_CPU:
        reg_value = readl(L3_VOL_DROP_EN_ADDR(base));
        writel(reg_value|L3_VOL_DROP_EN_BIT, L3_VOL_DROP_EN_ADDR(base));
        pr_err("[%s]enable cpu l3!\n", __func__);
        break;

    case GPU_CPU:
        reg_value = readl(GPU_VOL_DROP_EN_ADDR(base));
        writel(reg_value|GPU_VOL_DROP_EN_BIT, GPU_VOL_DROP_EN_ADDR(base));
        pr_err("[%s]enable cpu gpu!\n", __func__);
        break;

    case ALL:
        reg_value = readl(BIG_VOL_DROP_EN_ADDR(base));
        writel(reg_value|BIG_VOL_DROP_EN_BIT, BIG_VOL_DROP_EN_ADDR(base));

        reg_value = readl(MIDDLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value|MIDDLE_VOL_DROP_EN_BIT, MIDDLE_VOL_DROP_EN_ADDR(base));

        reg_value = readl(LITTLE_VOL_DROP_EN_ADDR(base));
        writel(reg_value|LITTLE_VOL_DROP_EN_BIT, LITTLE_VOL_DROP_EN_ADDR(base));

        reg_value = readl(L3_VOL_DROP_EN_ADDR(base));
        writel(reg_value|L3_VOL_DROP_EN_BIT, L3_VOL_DROP_EN_ADDR(base));

        reg_value = readl(GPU_VOL_DROP_EN_ADDR(base));
        writel(reg_value|GPU_VOL_DROP_EN_BIT, GPU_VOL_DROP_EN_ADDR(base));

        pr_err("%s]set cpu all!\n", __func__);
        break;

    default:
        pr_err("%s]set cpu nothing!\n", __func__);
        break;
    }

}

/*******************************************************
  Function:        vbat_drop_vol_set
  Description:     set vbat drop vol interrupt Threshold value
  Input:           mv :vbat drop interrupt  Threshold value
  Output:          NULL
  Return:          NULL
  Rmark:           NA
********************************************************/
STATIC void  hisi_vbat_drop_vol_set(int mv)
{
    unsigned char reg_val;

    if (mv > 3400)
        reg_val = 7;
    else if (mv > 3300)
        reg_val = 6;
    else if (mv > 3200)
        reg_val = 5;
    else if (mv > 3100)
        reg_val = 4;
    else  if (mv > 3000)
        reg_val = 3;
    else if (mv > 2900)
        reg_val = 2;
    else if (mv > 2800)
        reg_val = 1;
    else
        reg_val = 0;

    HISI_VBAT_DROP_PMIC_REG_WRITE(PMIC_VSYS_DROP_VOL_SET, reg_val);
    pr_info("[%s]:set vol [%d]mv reg [%d]!\n",__func__,mv, reg_val);

}

/*******************************************************
  Function:        hisi_vbat_drop_protect_probe
  Description:     probe function
  Input:           struct platform_device *pdev --- platform device
  Output:          NULL
  Return:          0:success,other:fail.
  Rmark:           enable cpu/gpu auto div, registe irq.
********************************************************/
static int hisi_vbat_drop_protect_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_vbat_drop_protect_dev *di = NULL;
	int ret = 0;

	di = devm_kzalloc(dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(dev, "cannot allocate hisi_vbat_drop_protect device info\n");
		return -ENOMEM;
	}

	di->dev = dev;
    platform_set_drvdata(pdev, di);

    /*get dts info*/
    ret = hisi_vbat_drop_parse_dts(di);
    if (ret)
        goto vbat_drop_err;

    /*request vbat drop vol irq*/
    di->vbat_drop_irq = platform_get_irq(pdev, 0);
    if (di->vbat_drop_irq < 0) {
        dev_err(dev, "IRQ resource(%d) is not available\n", di->vbat_drop_irq);
        ret = -1;
        goto  vbat_drop_err;
    }

    /*mask && clear old IRQ status*/
	HISI_VBAT_DROP_PMIC_REG_WRITE(PMIC_VSYS_DROP_IRQ_MASK_REG, PMIC_VSYS_DROP_IRQ_MASK);
    HISI_VBAT_DROP_PMIC_REG_WRITE(PMIC_VSYS_DROP_IRQ_REG, PMIC_VSYS_DROP_IRQ_CLEAR);

    ret = devm_request_irq(dev, di->vbat_drop_irq, hisi_vbat_drop_irq_handler, IRQF_TRIGGER_RISING, "vbat_drop", di);
	if (ret) {
		dev_err(dev, "could not claim vbat drop irq with err:%d\n", ret);
		ret = -1;
		goto vbat_drop_err;
	}
    /*init vbat drop work*/
    INIT_DELAYED_WORK(&di->vbat_drop_irq_work, hisi_vbat_drop_interrupt_work);

    wake_lock_init(&di->vbatt_check_lock, WAKE_LOCK_SUSPEND, "vbatt_drop_check_wake");

#ifdef CONFIG_HISI_HW_VOTE
	hisi_vbat_drop_cluster_freq_limit_init(dev);
#endif
    /*set vbat drop vol*/
    hisi_vbat_drop_vol_set(di->vbat_drop_vol_mv);

    /*enable cpu/gpu auto drop*/
    if (di->big_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, BIG_CPU, 1);
    if (di->middle_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, MIDDLE_CPU, 1);
    if (di->little_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, LITTLE_CPU, 1);
    if (di->l3_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, L3_CPU, 1);
    if (di->gpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, GPU_CPU, 1);

    /*unmask pmu vsys_drop interrupt*/
    HISI_VBAT_DROP_PMIC_REG_WRITE(PMIC_VSYS_DROP_IRQ_MASK_REG, ~PMIC_VSYS_DROP_IRQ_MASK);

    dev_err(dev, "[%s] probe success!\n", __func__);
	return 0;

vbat_drop_err:
    platform_set_drvdata(pdev, NULL);
    dev_err(dev, "[%s] probe fail!\n", __func__);
	return ret;
}

static int hisi_vbat_drop_protect_remove(struct platform_device *pdev)
{
	struct hisi_vbat_drop_protect_dev *di = platform_get_drvdata(pdev);

	if (!di) {
		pr_err("%s:di is NULL\n", __func__);
		return -ENOMEM;
	}

	devm_free_irq(di->dev, di->vbat_drop_irq, di);
	devm_kfree(&pdev->dev, di);
	return 0;
}

#ifdef CONFIG_PM
static int hisi_vbat_drop_protect_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	struct hisi_vbat_drop_protect_dev *di = platform_get_drvdata(pdev);

	if (!di) {
		pr_err("[%s]vbat drop dev is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s: suspend +\n", __func__);

   /*cancle cpu/gpu auto div*/
    hisi_vbat_drop_cpu_drop_en(di, ALL, 0);

	pr_info("%s: suspend -\n", __func__);

    return 0;
}

static int hisi_vbat_drop_protect_resume(struct platform_device *pdev)
{
	struct hisi_vbat_drop_protect_dev *di = platform_get_drvdata(pdev);

	if (!di) {
		pr_err("%s:di is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s: resume +\n", __func__);

    /*enable cpu/gpu auto div*/
    if (di->big_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, BIG_CPU, 1);
    if (di->middle_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, MIDDLE_CPU, 1);
    if (di->little_cpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, LITTLE_CPU, 1);
    if (di->l3_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, L3_CPU, 1);
    if (di->gpu_auto_div_en)
        hisi_vbat_drop_cpu_drop_en(di, GPU_CPU, 1);

	pr_info("%s: resume -\n", __func__);
	return 0;
}
#endif


static struct of_device_id hisi_vbat_drop_protect_of_match[] = {
	{.compatible = "hisi,vbat_drop_protect",},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_vbat_drop_protect_of_match);

static struct platform_driver hisi_vbat_drop_protect_driver = {
	.probe = hisi_vbat_drop_protect_probe,
	.remove = hisi_vbat_drop_protect_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "hisi_vbat_drop_prot",
		   .of_match_table = hisi_vbat_drop_protect_of_match,
		   },
#ifdef CONFIG_PM
	.suspend = hisi_vbat_drop_protect_suspend,
	.resume = hisi_vbat_drop_protect_resume,
#endif
};

module_platform_driver(hisi_vbat_drop_protect_driver);

MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("Vbat drop protect driver");
MODULE_LICENSE("GPL v2");
