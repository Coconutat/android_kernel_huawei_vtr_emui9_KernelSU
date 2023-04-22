/*
 * Device driver for regulators in Hi3xxx IC
 *
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (c) 2011 Hisilicon.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/slab.h>
#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/hisi_ip_core.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/clk-private.h>
#endif
#include <linux/interrupt.h>
#include <linux/hwspinlock.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/clk-provider.h>

#include <soc_crgperiph_interface.h>
#ifdef CONFIG_HISI_PMIC_DEBUG
#include <linux/debugfs.h>
#endif
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#if CONFIG_DEBUG_FS
#define IP_REGULATOR_DEBUG(args...) pr_debug(args);
#else
#define IP_REGULATOR_DEBUG(args...)
#endif

#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
extern void enable_err_probe_by_name(const char *name);
#endif

#ifdef CONFIG_HISI_FREQ_AUTODOWN
extern int hisi_freq_autodown_clk_set(char *freq_name, u32 control_flag);
#endif

#if defined CONFIG_MIA_IP_PLATFORM
enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	IVP_ID,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	VDEC_ID,
	VENC_ID,
	G3D_ID,
	ASP_ID,
};
#elif defined CONFIG_BOST_IP_PLATFORM
enum ip_regulator_id {
	MEDIA_SUBSYS_ID = 0,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	IVP_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	VDEC_ID,
	VENC_ID,
	ISP_SRT_ID,
	G3D_ID,
	ASP_ID,
	MMBUF_ID,
};
#elif defined CONFIG_ATLA_IP_PLATFORM
enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	VDEC_ID,
	VENC_ID,
	ICS_ID,
	ISP_R8_ID,
	G3D_ID,
	ASP_ID,
	ICS2_ID,
};
#elif defined CONFIG_PHOE_IP_PLATFORM
enum ip_regulator_id {
	VIVOBUS_ID = 0,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	VDEC_ID,
	VENC_ID,
	ISP_R8_ID,
	VENC2_ID,
	HIFACE_ID,
	MEDIA1_SUBSYS_ID,
	MEDIA2_SUBSYS_ID,
	NPU_ID,
	G3D_ID,
	ASP_ID,
};
#elif defined CONFIG_IP_PLATFORM_COMMON
enum ip_regulator_id {
	MEDIA1_SUBSYS_ID = 0,
	MEDIA2_SUBSYS_ID,
	VIVOBUS_ID,
	VCODECSUBSYS_ID,
	DSSSUBSYS_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	VDEC_ID,
	VENC_ID,
	ISP_R8_ID,
	ASP_ID,
	G3D_ID,
	ICS_ID,
	ICS2_ID,
	NPU_ID,
};
#else
enum ip_regulator_id {
	VIVOBUS_ID = 0,
	DSSSUBSYS_ID,
	VCODECSUBSYS_ID,
	VDEC_ID,
	VENC_ID,
	ISPSUBSYS_ID,
	IVP_ID,
	DEBUGSUBSYS_ID,
};
#endif

/******************************macro parameter*********************************/
enum {
	REGULATOR_DISABLE = 0,
	REGULATOR_ENABLE
};

#define AP_TO_LPM_MSG_NUM           2
#define REGULATOR_HWLOCK_TIMEOUT    1000
#define AP_AOD_EN                   0
#define SENSORHUB_AOD_EN            1

static struct hwspinlock	*regulator_hwlock_29;

/*****************************ip struct parameter*****************************/
struct hisi_regulator_ip {
	const char *name;
	struct device_node *np;
	struct regulator_desc rdesc;
	int (*dt_parse)(struct hisi_regulator_ip *, struct platform_device *);
	int regulator_id;
	struct mutex regulator_lock;
	u32 lpm_enable_value[AP_TO_LPM_MSG_NUM];
	u32 lpm_disable_value[AP_TO_LPM_MSG_NUM];
	u32 regulator_enalbe_flag;
	int regulator_fake;
	int regulator_type;
	int clock_num;
	int clock_check_flag;
	int enable_clock;
	int clock_set_rate_flag;
	unsigned int *clock_rate_value;
	unsigned int *clock_org_rate;
	int enable_autofreqdump;
	unsigned int ppll0_clock_set_rate_flag;
	unsigned int ppll0_clock_rate_value;
	void __iomem *sctrl_reg;
	int hwlock_seq;	/* aod hwlock sequnce*/
	u32 aod_sc_offset; /* SC offset */
#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
	const char *noc_node_name;
#endif
#if defined (CONFIG_BOST_IP_PLATFORM) || defined(CONFIG_MIA_IP_PLATFORM) || defined(CONFIG_ATLA_IP_PLATFORM) || defined(CONFIG_PHOE_IP_PLATFORM) || defined(CONFIG_IP_PLATFORM_COMMON)
	u32 dss_boot_check[2];
#endif
};

#if defined (CONFIG_BOST_IP_PLATFORM) || defined(CONFIG_MIA_IP_PLATFORM) || defined(CONFIG_ATLA_IP_PLATFORM) || defined(CONFIG_PHOE_IP_PLATFORM) || defined(CONFIG_IP_PLATFORM_COMMON)
#define DSS_SOFTRESET_STATE_CHECK_BIT			0
#else
#define DSS_SOFTRESET_STATE_CHECK_BIT			SOC_CRGPERIPH_PERRSTEN3_ip_rst_dss_START
#endif
/**************************control interface***************************************/
#ifdef CONFIG_HISI_FREQ_AUTODOWN
int hisi_regulator_freq_autodown_clk(int regulator_id, u32 flag)
{
	int ret = 0;

	IP_REGULATOR_DEBUG("<[%s]: begin regulator_id=%d, flag=%d>\n", __func__, regulator_id, flag);
	switch (regulator_id) {
	case VIVOBUS_ID:
		ret = hisi_freq_autodown_clk_set("vivobus", flag);
		break;
	case VCODECSUBSYS_ID:
		ret = hisi_freq_autodown_clk_set("vcodecbus", flag);
		break;
	case VDEC_ID:
		ret = hisi_freq_autodown_clk_set("vdecbus", flag);
		break;
	case VENC_ID:
		ret = hisi_freq_autodown_clk_set("vencbus", flag);
		break;
	case ISPSUBSYS_ID:
		ret = hisi_freq_autodown_clk_set("ispa7bus", flag);
		break;
	case IVP_ID:
		ret = hisi_freq_autodown_clk_set("ivpbus", flag);
		break;
#if defined(CONFIG_ATLA_IP_PLATFORM)
	case ICS_ID:
		ret = hisi_freq_autodown_clk_set("icsbus", flag);
		break;
	case ICS2_ID:
		ret = hisi_freq_autodown_clk_set("ics2bus", flag);
		break;
#endif
#if defined(CONFIG_IP_PLATFORM_COMMON)
	case NPU_ID:
		ret = hisi_freq_autodown_clk_set("npubus", flag);
		break;
#endif
#if defined(CONFIG_PHOE_IP_PLATFORM)
	case VENC2_ID:
		ret = hisi_freq_autodown_clk_set("venc2bus", flag);
		break;
#endif
	default:
		pr_err("[%s]:Input regulator ID is not exist.", __func__);
		return -1;
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, regulator_id);
	return ret;
}
#endif

/*Always on display need to vote media1,vivobus power on/off in sctrl*/
static int aod_set_and_get_poweron_state(struct hisi_regulator_ip *sreg, u32 control_flag)
{
	int ret = 0;
	int bit_mask = 0;
	int val = 0;
	/*sctrl 0x438 : power state vote
	*bit[0]:AP			media1 vote
	*bit[1]:Sensorhub	media1 vote
	*bit[2]:AP			vivobus vote
	*bit[3]:Sensorhub	vivobus vote
	*bit[4]:AP			dss vote
	*bit[5]:Sensorhub	dss vote
	*/
	if (VIVOBUS_ID == sreg->regulator_id) {
		bit_mask = 2;
	} else if (DSSSUBSYS_ID == sreg->regulator_id)	{
		bit_mask = 4;
	}

	val = readl(sreg->sctrl_reg + sreg->aod_sc_offset);
	if (0 == (val & (BIT(SENSORHUB_AOD_EN)<<bit_mask)))
		ret = 0;
	else
		ret = 1;
	if (REGULATOR_ENABLE == control_flag) {
		/* vote AP media/vivobus power up */
		val |= (BIT(AP_AOD_EN)<<bit_mask);
	} else {
		/* vote AP media/vivobus power off */
		val &= ~(BIT(AP_AOD_EN)<<bit_mask);
	}
	writel(val, sreg->sctrl_reg + sreg->aod_sc_offset);

	return ret;
}

static inline struct hisi_regulator_ip_core *rdev_to_ip_core(struct regulator_dev *dev)
{
	/* regulator_dev parent to->
	 * hisi ip regulator platform device_dev parent to->
	 * hisi ip pmic platform device_dev
	 */
	return dev_get_drvdata(rdev_get_dev(dev)->parent->parent);
}

extern bool __clk_is_enabled(struct clk *clk);
static struct clk *of_regulator_clk_get(struct device_node *node, int index)
{
	struct clk *clk;
	const char *clk_name;
	int ret = 0;
	IP_REGULATOR_DEBUG("<[%s]: begin>\n", __func__);
	ret = of_property_read_string_index(node, "clock-names", index, &clk_name);
	if (ret != 0) {
		pr_err("failed to get regulator clk resource! ret=%d.\n", ret);
		return ERR_PTR(-EINVAL);
	}
	clk = clk_get(NULL, clk_name);
	if (IS_ERR(clk)) {
		pr_err(" %s: %s %d get failed!\n", __func__, clk_name, __LINE__);
		return ERR_PTR(-EINVAL);
	}
	IP_REGULATOR_DEBUG("<[%s]: end>\n", __func__);
	return clk;
}

static int hisi_clock_state_check(struct hisi_regulator_ip *sreg)
{
	struct clk *temp_clock;
	int i;
	bool clock_flag;
	IP_REGULATOR_DEBUG("<[%s]: begin regulator_id=%d>\n", __func__, sreg->regulator_id);
	for (i = 0; i < sreg->clock_num; i++) {
		temp_clock = of_regulator_clk_get(sreg->np, i);
		if (IS_ERR(temp_clock)) {
			temp_clock = NULL;
			return -EINVAL;
		}
		IP_REGULATOR_DEBUG("<[%s]: clock_name=%s>\n", __func__, __clk_get_name(temp_clock));
		clock_flag = __clk_is_enabled(temp_clock);
		if (false == clock_flag) {
			pr_err("<[%s]: clock_name=%s had closed!>\n", __func__, __clk_get_name(temp_clock));
		}
		clk_put(temp_clock);
		temp_clock = NULL;
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return 0;
}
#if defined (CONFIG_BOST_IP_PLATFORM) || defined(CONFIG_MIA_IP_PLATFORM) || defined(CONFIG_ATLA_IP_PLATFORM) || defined(CONFIG_PHOE_IP_PLATFORM) || defined(CONFIG_IP_PLATFORM_COMMON)
static int get_softreset_state(struct hisi_regulator_ip_core *pmic, struct hisi_regulator_ip *sreg, unsigned int value)
{
	int ret = 0;
	int val = 0;
	/*First boot and AOD to camera need check DSS poweron status*/
	if (sreg->hwlock_seq > 0) {
		if (NULL == regulator_hwlock_29) {
			pr_err("[%s]regulator hwlock_29 hwspinlock is null!\n", __func__);
			return -ENOENT;
		}

		if (hwspin_lock_timeout(regulator_hwlock_29, REGULATOR_HWLOCK_TIMEOUT)) {
			pr_err("Aod regulator enable hwspinlock timout!\n");
			return -ENOENT;
		}
	}
	IP_REGULATOR_DEBUG("<[%s]: sreg->dss_boot_check[0]=0x%x>\n", __func__, sreg->dss_boot_check[0]);
	IP_REGULATOR_DEBUG("<[%s]: regulator_id[%d] softreset_value=0x%x softreset_state3 = 0x%x>\n", __func__,
					sreg->regulator_id, sreg->dss_boot_check[1], readl((sreg->dss_boot_check[0]+ pmic->regs)));
	if (((sreg->dss_boot_check[1]) & (readl(sreg->dss_boot_check[0]+ pmic->regs))))
		ret = 0;/*softreset*/
	else
		ret = 1;/*unreset*/
	if (sreg->hwlock_seq > 0) {
		if(ret == 1){
			val = readl(sreg->sctrl_reg + sreg->aod_sc_offset);
			val |= (BIT(4));
			pr_err("[%s]AOD DSS vote val = 0x%x !\n", __func__, val);
			writel(val, sreg->sctrl_reg + sreg->aod_sc_offset);/*vote aod dss enable*/
		}
		hwspin_unlock(regulator_hwlock_29);
	}
	return ret;
}/*lint !e715*/
#else
static int get_softreset_state(struct hisi_regulator_ip_core *pmic, struct hisi_regulator_ip *sreg, unsigned int value)
{
	IP_REGULATOR_DEBUG("<[%s]: regulator_id[%d] softreset_bit=%d softreset_state3 = 0x%x>\n", __func__,
					sreg->regulator_id, value, readl(SOC_CRGPERIPH_PERRSTSTAT3_ADDR(pmic->regs)));
	if (((1 << value) & (readl(SOC_CRGPERIPH_PERRSTSTAT3_ADDR(pmic->regs)))))
		return 0;/*softreset*/
	else
		return 1;/*unreset*/
}
#endif
static int hisi_ppll0_clock_rate_set(struct hisi_regulator_ip *sreg)
{
	struct clk *temp_clock;
	int ret;

	IP_REGULATOR_DEBUG("<[%s]: begin regulator_id=%d clock_num=%d>\n", __func__, sreg->regulator_id, sreg->clock_num);
	/*0:get current clock*/
	temp_clock = of_regulator_clk_get(sreg->np, 0);
	if (IS_ERR(temp_clock)) {
		temp_clock = NULL;
		return -EINVAL;
	}
	IP_REGULATOR_DEBUG("<[%s]: clock_name=%s, clock_rate=0x%x>\n", __func__, __clk_get_name(temp_clock), sreg->ppll0_clock_rate_value);
	/*1:set freq rate to clock*/
	ret = clk_set_rate(temp_clock, sreg->ppll0_clock_rate_value);
	if (ret < 0) {
		pr_err("file:%s line:%d temp clock[%s] set rate fail!\n", __func__, __LINE__, __clk_get_name(temp_clock));
		ret = -EINVAL;
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	clk_put(temp_clock);
	temp_clock = NULL;
	return ret;
}

static int hisi_clock_rate_set(struct hisi_regulator_ip *sreg, int flag)
{
	struct clk *temp_clock;
	int i;
	int ret;

	IP_REGULATOR_DEBUG("<[%s]: begin regulator_id=%d clock_num=%d>\n", __func__, sreg->regulator_id, sreg->clock_num);
	if (0 == flag) {/*save*/
		for (i = 0; i < sreg->clock_num; i++) {
			if (0 == sreg->clock_rate_value[i])
				continue;
			/*0:get current clock*/
			temp_clock = of_regulator_clk_get(sreg->np, i);
			if (IS_ERR(temp_clock)) {
				temp_clock = NULL;
				return -EINVAL;
			}
			IP_REGULATOR_DEBUG("<[%s]: clock_name=%s>\n", __func__, __clk_get_name(temp_clock));
			/*1:get clock org value*/
			sreg->clock_org_rate[i] = clk_get_rate(temp_clock);
			/*2:set low freq rate to clock*/
			ret = clk_set_rate(temp_clock, sreg->clock_rate_value[i]);
			if (ret < 0) {
				pr_err("file:%s line:%d temp clock[%s] set rate fail!\n", __func__, __LINE__, __clk_get_name(temp_clock));
				goto clk_err_put;
			}
			IP_REGULATOR_DEBUG("<[%s]: begin clock_name = %s clock_org_rate[%d]=%d, clock_current_rate[%d]=%d>\n",
				__func__, __clk_get_name(temp_clock), i, sreg->clock_org_rate[i], i, sreg->clock_rate_value[i]);
			IP_REGULATOR_DEBUG("<[%s]: current clock_name[%s] rate[%ld]>\n", __func__,
							__clk_get_name(temp_clock), clk_get_rate(temp_clock));
			clk_put(temp_clock);
			temp_clock = NULL;
		}
	} else {
		for (i = 0; i < sreg->clock_num; i++) {
			if (0 == sreg->clock_rate_value[i])
				continue;
			/*0:get current clock*/
			temp_clock = of_regulator_clk_get(sreg->np, i);
			if (IS_ERR(temp_clock)) {
				temp_clock = NULL;
				return -EINVAL;
			}
			/*1:resume org clock rate*/
			ret = clk_set_rate(temp_clock, sreg->clock_org_rate[i]);
			if (ret < 0) {
				pr_err("file:%s line:%d temp clock[%s] set rate fail!\n", __func__, __LINE__, __clk_get_name(temp_clock));
				goto clk_err_put;
			}
			IP_REGULATOR_DEBUG("<[%s]: end clock_name = %s clock_org_rate[%d]=%d>\n",
				__func__, __clk_get_name(temp_clock), i, sreg->clock_org_rate[i]);
			IP_REGULATOR_DEBUG("<[%s]: current clock_name[%s] rate[%ld]>\n", __func__,
								__clk_get_name(temp_clock), clk_get_rate(temp_clock));
			clk_put(temp_clock);
			temp_clock = NULL;
		}
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return 0;
clk_err_put:
	clk_put(temp_clock);
	temp_clock = NULL;
	return -EINVAL;
}

static int hisi_ip_clock_work(struct hisi_regulator_ip *sreg, int flag)
{
	struct clk *temp_clock;
	int i, ret = 0;
	IP_REGULATOR_DEBUG("<[%s]: begin regulator_id=%d, clock_num = %d>\n", __func__, sreg->regulator_id, sreg->clock_num);
	for (i = 0; i < sreg->clock_num; i++) {
		temp_clock = of_regulator_clk_get(sreg->np, i);
		if (IS_ERR(temp_clock)) {
			temp_clock = NULL;
			return -EINVAL;
		}
		IP_REGULATOR_DEBUG("<[%s]: clock_name = %s>\n", __func__, __clk_get_name(temp_clock));
		/*flag: 0 disable clock; 1 enable clock*/
		if (flag == REGULATOR_ENABLE) {
			ret = clk_prepare_enable(temp_clock);
			if (ret) {
				pr_err("Regulator hi3630:regulator_id[%d],clock_id[%d] enable failed\r\n", sreg->regulator_id, i);
				clk_put(temp_clock);
				temp_clock = NULL;
				return ret;
			}
			IP_REGULATOR_DEBUG("<[%s]: clock_name = %s enable>\n", __func__, __clk_get_name(temp_clock));
		} else {
			IP_REGULATOR_DEBUG("<[%s]: clock_name = %s disable>\n", __func__, __clk_get_name(temp_clock));
			clk_disable_unprepare(temp_clock);
		}
		clk_put(temp_clock);
		temp_clock = NULL;
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return ret;
}

/**************************arm trust firmware****************************************/
#define IP_REGULATOR_REGISTER_FN_ID          (0xc500fff0)
noinline int atfd_hisi_service_ip_regulator_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			"smc    #0\n"
			: "+r" (function_id)
			: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

/**************************lpmcu****************************************/
static int hisi_ip_to_atf_is_enabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	struct hisi_regulator_ip_core *pmic = rdev_to_ip_core(dev);

	if (sreg == NULL || pmic == NULL) {
		pr_err("[%s]regulator get  para is err!\n", __func__);
		return -EINVAL;
	}
	IP_REGULATOR_DEBUG("<[%s]:regulator_id=%d>\n", __func__, sreg->regulator_id);
	return sreg->regulator_enalbe_flag;
}

static int hisi_ip_power_on(struct hisi_regulator_ip *sreg)
{
	int ret = 0;
	int aod_ret = 0;
	if (sreg->hwlock_seq > 0) {
		if (NULL == regulator_hwlock_29) {
			pr_err("[%s] regulator hwlock_29 hwspinlock is null!\n", __func__);
			return -ENOENT;
		}

		if (hwspin_lock_timeout(regulator_hwlock_29, REGULATOR_HWLOCK_TIMEOUT)) {
			pr_err("Aod regulator enable hwspinlock timout!\n");
			return -ENOENT;
		}
		aod_ret = aod_set_and_get_poweron_state(sreg, REGULATOR_ENABLE);
		if (0 == aod_ret) {
			ret = atfd_hisi_service_ip_regulator_smc(IP_REGULATOR_REGISTER_FN_ID, sreg->regulator_id, REGULATOR_ENABLE, 0);
		} else if (1 == aod_ret) {
			ret = 0;
		} else {
			pr_err("%s:aod set and get poweron state [%s] failled!\n\r", __func__, sreg->name);
		}
		hwspin_unlock(regulator_hwlock_29);
	} else {
		ret = atfd_hisi_service_ip_regulator_smc(IP_REGULATOR_REGISTER_FN_ID, sreg->regulator_id, REGULATOR_ENABLE, 0);
	}
	return ret;
}

static int hisi_ip_power_off(struct hisi_regulator_ip *sreg)
{
	int ret = 0;
	int aod_ret = 0;
	if (sreg->hwlock_seq > 0) {
		if (NULL == regulator_hwlock_29) {
			pr_err("[%s] regulator hwlock_29 hwspinlock is null!\n", __func__);
			return -ENOENT;
		}
		if (hwspin_lock_timeout(regulator_hwlock_29, REGULATOR_HWLOCK_TIMEOUT)) {
			pr_err("Aod regulator enable hwspinlock timout!\n");
			return -ENOENT;
		}
		aod_ret = aod_set_and_get_poweron_state(sreg, REGULATOR_DISABLE);
		if (0 == aod_ret)
			ret = atfd_hisi_service_ip_regulator_smc(IP_REGULATOR_REGISTER_FN_ID, sreg->regulator_id, REGULATOR_DISABLE, 0);
		else if (1 == aod_ret)
			ret = 0;
		else
			pr_err("%s:aod set and get poweroff state [%s] failled!\n\r", __func__, sreg->name);
		hwspin_unlock(regulator_hwlock_29);
	} else {
		ret = atfd_hisi_service_ip_regulator_smc(IP_REGULATOR_REGISTER_FN_ID, sreg->regulator_id, REGULATOR_DISABLE, 0);
	}
	return ret;
}

static int hisi_ip_to_atf_enabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	struct hisi_regulator_ip_core *pmic = rdev_to_ip_core(dev);
	int ret = 0;
	int softreset_value;
	int dss_poweron_flag = 0;

	if (sreg == NULL || pmic == NULL) {
		pr_err("[%s]regulator get  para is err!\n", __func__);
		return -EINVAL;
	}

	IP_REGULATOR_DEBUG("\n<[%s]: begin regulator_id=%d>\n", __func__, sreg->regulator_id);
	if (sreg->regulator_fake) {
		sreg->regulator_enalbe_flag = 1;
		return 0;
	}

	/*this patch was resolved for dss power on in fastboot phase*/
	if ((DSSSUBSYS_ID == sreg->regulator_id)) {
		softreset_value = get_softreset_state(pmic, sreg, DSS_SOFTRESET_STATE_CHECK_BIT);
		IP_REGULATOR_DEBUG("<[%s]: regulator_id[%d] softreset_bit=%d>\n", __func__, sreg->regulator_id, softreset_value);
		if (softreset_value) {
			IP_REGULATOR_DEBUG("<[%s]: regulator_id[%d] was poweron in fastboot phase>\n", __func__, sreg->regulator_id);
			dss_poweron_flag = 1;
			sreg->regulator_enalbe_flag = 1;
			return 0;
		}
	}

	if ((1 == sreg->clock_set_rate_flag) && (dss_poweron_flag == 0)) {
		ret  = hisi_clock_rate_set(sreg, 0);
		if (ret)
			pr_err("%s:hisi clock rate set fail!\n", __func__);
	}

	if (1 == sreg->clock_check_flag) {
		ret  = hisi_clock_state_check(sreg);
		if (ret)
			pr_err("%s:hisi clock state check fail!\n", __func__);

	}

	if (1 == sreg->enable_clock) {
		ret = hisi_ip_clock_work(sreg, REGULATOR_ENABLE);
		if (ret) {
			pr_err("[%s]hisi hisi_ip_clock_work is failed. ret[%d]!\n", __func__, ret);
		}
	}

#ifdef CONFIG_HISI_FREQ_AUTODOWN
	if (1 == sreg->enable_autofreqdump) {
		ret = hisi_regulator_freq_autodown_clk(sreg->regulator_id, REGULATOR_DISABLE);
		if (ret) {
			pr_err("[%s]hisi freq auto down is failed. ret[%d]!\n", __func__, ret);
		}
	}
#endif
	ret = hisi_ip_power_on(sreg);
	if (0 == ret) {
#ifdef CONFIG_HISI_FREQ_AUTODOWN
		if (1 == sreg->enable_autofreqdump) {
			ret = hisi_regulator_freq_autodown_clk(sreg->regulator_id, REGULATOR_ENABLE);
			if (ret) {
				pr_err("[%s]hisi freq auto down is failed. ret[%d]!\n", __func__, ret);
			}
		}
#endif
		sreg->regulator_enalbe_flag = 1;
	} else {
		pr_err("%s:hisi ip send enable ldo[%s] to atf failled!\n\r", __func__, sreg->name);
	}

#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
	/* enable noc errprobe */
	if (sreg->noc_node_name)
		enable_err_probe_by_name(sreg->noc_node_name);
#endif

	if ((1 == sreg->clock_set_rate_flag) && (dss_poweron_flag == 0)) {
		ret = hisi_clock_rate_set(sreg, 1);
		if (ret)
			pr_err("%s:hisi clock rate set fail!\n", __func__);
	}

	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return ret;
}

static int hisi_ip_to_atf_disabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	int ret = 0;

	if (sreg == NULL) {
		pr_err("[%s]regulator get  para is err!\n", __func__);
		return -EINVAL;
	}

	IP_REGULATOR_DEBUG("\n<[%s]: begin regulator_id=%d>\n", __func__, sreg->regulator_id);

	if (sreg->regulator_fake) {
		sreg->regulator_enalbe_flag = 0;
		return 0;
	}

	if (sreg->clock_check_flag) {
		ret = hisi_clock_state_check(sreg);
		if (ret)
			pr_err("%s:hisi clock state check fail!\n", __func__);
	}

#ifdef CONFIG_HISI_FREQ_AUTODOWN
	if (1 == sreg->enable_autofreqdump) {
		ret = hisi_regulator_freq_autodown_clk(sreg->regulator_id, REGULATOR_DISABLE);
		if (ret) {
			pr_err("[%s]hisi freq auto down is failed. ret[%d]!\n", __func__, ret);
		}
	}
#endif
	ret = hisi_ip_power_off(sreg);
	if (!ret) {
		sreg->regulator_enalbe_flag = 0;
		ret = 0;
	} else {
		pr_err("%s:hisi ip send disable ldo[%s] to atf failled!\n\r", __func__, sreg->name);
	}

	if (1 == sreg->ppll0_clock_set_rate_flag) {
		ret = hisi_ppll0_clock_rate_set(sreg);
		if (ret) {
			pr_err("[%s]hisi ppll0_clock_set_rate_flag set failed. ret[%d]!\n", __func__, ret);
		}
	}
	/*AOD need vivobus always on*/
	if (1 == sreg->enable_clock) {
		ret = hisi_ip_clock_work(sreg, REGULATOR_DISABLE);
		if (ret) {
			pr_err("[%s]hisi hisi_ip_clock_work is failed. ret[%d]!\n", __func__, ret);
		}
	}

	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return ret;
}
/**************************lpmcu****************************************/
#if defined(CONFIG_HISI_RPROC) || defined(CONFIG_HISI_IP_REGULATOR_IPC)
static int hisi_ip_regulator_cmd_send(struct regulator_dev *dev, int cmd)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	rproc_msg_t ack_buffer[2] = {0};
	rproc_msg_t *tx_buffer = NULL;
	int err = 0;

	if (sreg == NULL) {
		pr_err("[%s]regulator get  para is err!\n", __func__);
		return -EINVAL;
	}

	if (REGULATOR_ENABLE == cmd)
		tx_buffer = sreg->lpm_enable_value;
	else
		tx_buffer = sreg->lpm_disable_value;

	IP_REGULATOR_DEBUG("<[%s]: send msg to tx_buffer[0]=0x%x>\n", __func__, tx_buffer[0]);
	IP_REGULATOR_DEBUG("<[%s]: send msg to tx_buffer[1]=0x%x>\n", __func__, tx_buffer[1]);

	err = RPROC_SYNC_SEND(HISI_RPROC_LPM3_MBX14, tx_buffer,
		AP_TO_LPM_MSG_NUM, ack_buffer, AP_TO_LPM_MSG_NUM);
	if (err || (ack_buffer[0] != tx_buffer[0]) || ((ack_buffer[1] >> 24) != 0x0)) {
		pr_err("%s: regulator ID[%d] rproc sync send err, err = %d, ack_buffer[0] = %u, ack_buffer[1] = %u!\n",
						__func__, sreg->regulator_id, err, ack_buffer[0], ack_buffer[1]);
		return -EINVAL;
	}

	return 0;
}

static int hisi_ip_to_lpm_is_enabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	struct hisi_regulator_ip_core *pmic = rdev_to_ip_core(dev);

	if (sreg == NULL || pmic == NULL) {
		pr_err("[%s]regulator get  para is err!\n", __func__);
		return -EINVAL;
	}

	return sreg->regulator_enalbe_flag;
}

static int hisi_ip_to_lpm_enabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	int ret = 0;
	IP_REGULATOR_DEBUG("\n<[%s]: begin regulator_id=%d>\n", __func__, sreg->regulator_id);

	if (sreg->regulator_fake) {
		sreg->regulator_enalbe_flag = 1;
		return 0;
	}

	ret = hisi_ip_regulator_cmd_send(dev, REGULATOR_ENABLE);
	if (!ret) {
		sreg->regulator_enalbe_flag = 1;
	} else {
		pr_err("%s:hisi ip send enable ldo[%s] to lpm failled!\n\r", __func__, sreg->name);
	}
#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
	/* enable noc errprobe */
	if (sreg->noc_node_name)
		enable_err_probe_by_name(sreg->noc_node_name);
#endif
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return ret;
}

static int hisi_ip_to_lpm_disabled(struct regulator_dev *dev)
{
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(dev);
	int ret = 0;

	IP_REGULATOR_DEBUG("\n<[%s]: begin regulator_id=%d>\n", __func__, sreg->regulator_id);

	if (sreg->regulator_fake) {
		sreg->regulator_enalbe_flag = 0;
		return 0;
	}

	ret = hisi_ip_regulator_cmd_send(dev, REGULATOR_DISABLE);
	if (!ret || (-ETIMEOUT == ret)) {
		sreg->regulator_enalbe_flag = 0;
		ret = 0;
	} else {
		pr_err("%s:hisi ip send disable ldo[%s] to lpm failled!\n\r", __func__, sreg->name);
	}
	IP_REGULATOR_DEBUG("<[%s]: end regulator_id=%d>\n", __func__, sreg->regulator_id);
	return ret;
}
#else
static int hisi_ip_to_lpm_is_enabled(struct regulator_dev *dev)
{
	return 0;
}

static int hisi_ip_to_lpm_enabled(struct regulator_dev *dev)
{
	return 0;
}
static int hisi_ip_to_lpm_disabled(struct regulator_dev *dev)
{
	return 0;
}
#endif


#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
static int hisi_noc_dt_parse(struct hisi_regulator_ip *sreg,
				struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int ret = 0;

	/* noc_node_name */
	ret = of_property_read_string(np, "hisi,noc-node-name", &sreg->noc_node_name);
	if (ret) {
		pr_debug("no hisi,noc-node-name property found\n");
	}

	return ret;
}
#endif

static int hisi_dt_parse_aod_atf(struct hisi_regulator_ip *sreg, struct device_node *np)
{
	u32 aod_resource[2] = {0};
	if (NULL != of_find_property(np, "hisilicon,hisi-need-to-hwlock", NULL)) {
		if (of_property_read_u32_array(np, "hisilicon,hisi-need-to-hwlock", aod_resource, 2)) {
			pr_err("[%s]get hisilicon,hisi-need-to-hwlock attribute failed.\n", __func__);
			return -ENODEV;
		}
	}
	sreg->hwlock_seq = aod_resource[0];
	sreg->aod_sc_offset= aod_resource[1];
	if (sreg->hwlock_seq > 0) {
		if (NULL == regulator_hwlock_29) {
			regulator_hwlock_29 = hwspin_lock_request_specific(sreg->hwlock_seq);
			if (NULL == regulator_hwlock_29) {
				pr_err("Aod regulator request hwspin lock failed !\n");
				return -ENODEV;
			}
		}
	}
	return 0;
}

static int hisi_dt_parse_ip_atf(struct hisi_regulator_ip *sreg,
				struct platform_device *pdev)
{
	struct device *dev = NULL;
	struct device_node *np = NULL;
	int id = 0, fake = 0, type = 0;
	int ret = 0;
#if defined (CONFIG_BOST_IP_PLATFORM) || defined(CONFIG_MIA_IP_PLATFORM) || defined(CONFIG_ATLA_IP_PLATFORM) || defined(CONFIG_PHOE_IP_PLATFORM) || defined(CONFIG_IP_PLATFORM_COMMON)
	unsigned int register_info[2] = {0};
#endif

	if (sreg == NULL || pdev == NULL) {
		pr_err("[%s]regulator get  dt para is err!\n", __func__);
		return -EINVAL;
	}

	dev = &pdev->dev;
	np = dev->of_node;

	/* regulator-id */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-id",
						&id, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-id property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_id = id;

	/* regulator-type */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-type",
						&type, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-type property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_type = type;

	/* regulator-fake */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-is-fake",
						&fake, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-is-fake property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_fake = fake;

	/* clock_check_flag */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-clk-check-flag",
						&sreg->clock_check_flag, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-clk-check-flag property set\n");
		goto dt_parse_common_end;
	}

	/* clock_num*/
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-clk-num",
						&sreg->clock_num, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-clk-num property set\n");
		goto dt_parse_common_end;
	}

	/* enable clock flag*/
	ret = of_property_read_u32_array(np, "hisilicon,hisi-need-to-enable-clock",
						&sreg->enable_clock, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-need-to-enable-clock property set\n");
		goto dt_parse_common_end;
	}

	/* enable autofreqdump flag*/
	ret = of_property_read_u32_array(np, "hisilicon,hisi-need-to-autofreqdump",
						&sreg->enable_autofreqdump, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-need-to-autofreqdump property set\n");
		goto dt_parse_common_end;
	}

	/* clock set rate flag*/
	ret = of_property_read_u32_array(np, "hisilicon,hisi-clock-rate-set-flag",
						&sreg->clock_set_rate_flag, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-clock-rate-set-flag property set\n");
		goto dt_parse_common_end;
	}

	if (1 == sreg->clock_set_rate_flag) {
		/* clock rate*/
		sreg->clock_rate_value = (u32 *)devm_kzalloc(dev, sizeof(u32)*sreg->clock_num, GFP_KERNEL);
		if (!sreg->clock_rate_value) {
			dev_err(dev, "[%s]kzalloc clock_rate_value buffer failed.\n", __func__);
			return -ENOMEM;
		}

		sreg->clock_org_rate = (u32 *)devm_kzalloc(dev, sizeof(u32)*sreg->clock_num, GFP_KERNEL);
		if (!sreg->clock_org_rate) {
			dev_err(dev, "[%s]kzalloc clock_org_rate buffer failed.\n", __func__);
			return -ENOMEM;
		}

		ret = of_property_read_u32_array(np, "hisilicon,hisi-clock-rate-set",
							sreg->clock_rate_value, sreg->clock_num);
		if (ret) {
			dev_err(dev, "[%s]get hisilicon,hisi-clock-rate-set-flag attribute failed.\n", __func__);
			return -ENODEV;
		}
	}
	/*ppll0 clock set rate flag*/
	ret = of_property_read_u32_array(np, "hisilicon,hisi-ppll0-clock-rate-set-flag",
						&sreg->ppll0_clock_set_rate_flag, 1);
	if (ret) {
		sreg->ppll0_clock_set_rate_flag = 0;
		ret = 0;
	}

	IP_REGULATOR_DEBUG("<[%s]: regulator_id=%d, ppll0_clock_set_rate_flag=%d>\n",
							__func__, sreg->regulator_id, sreg->ppll0_clock_set_rate_flag);
	if (1 == sreg->ppll0_clock_set_rate_flag) {
		/* clock rate*/
		ret = of_property_read_u32_array(np, "hisilicon,hisi-ppll0-clock-rate-set",
							&sreg->ppll0_clock_rate_value, 1);
		if (ret) {
			dev_err(dev, "[%s]get hisilicon,hisi-ppll0-clock-rate-set-flag attribute failed.\n", __func__);
			return -ENODEV;
		}
	}

#if defined (CONFIG_BOST_IP_PLATFORM) || defined(CONFIG_MIA_IP_PLATFORM) || defined(CONFIG_ATLA_IP_PLATFORM) || defined(CONFIG_PHOE_IP_PLATFORM) || defined(CONFIG_IP_PLATFORM_COMMON)
	if ((DSSSUBSYS_ID == sreg->regulator_id)) {
		of_property_read_u32_array(np, "hisilicon,hisi-regulator-dss-boot-check",
							register_info, 2);
		sreg->dss_boot_check[0] = register_info[0];
		sreg->dss_boot_check[1] = register_info[1];
	}
#endif
	hisi_dt_parse_aod_atf(sreg, np);
#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
	hisi_noc_dt_parse(sreg, pdev);
#endif
dt_parse_common_end:
	return ret;
}

static int hisi_dt_parse_ip_lpm(struct hisi_regulator_ip *sreg,
				struct platform_device *pdev)
{
	struct device *dev = NULL;
	struct device_node *np = NULL;
	int id = 0, fake = 0, type = 0;
	unsigned int register_info[2] = {0};
	int ret = 0;

	if (sreg == NULL || pdev == NULL) {
		pr_err("[%s]regulator get  dt para is err!\n", __func__);
		return -EINVAL;
	}

	dev = &pdev->dev;
	np = dev->of_node;

	/* parse .hisi top cssys ctrl_reg */
	ret = of_property_read_u32_array(np, "ip_to_lpm_enable_step",
						register_info, AP_TO_LPM_MSG_NUM);
	if (ret) {
		dev_err(dev, "no hisilicon, ip to lpm enable property set\n");
		goto dt_parse_common_end;
	}
	sreg->lpm_enable_value[0] = register_info[0];
	sreg->lpm_enable_value[1] = register_info[1];

	/* parse .hisi top cssys ctrl_reg */
	ret = of_property_read_u32_array(np, "ip_to_lpm_disable_step",
						register_info, AP_TO_LPM_MSG_NUM);
	if (ret) {
		dev_err(dev, "no hisilicon,ip to lpm disable property set\n");
		goto dt_parse_common_end;
	}
	sreg->lpm_disable_value[0] = register_info[0];
	sreg->lpm_disable_value[1] = register_info[1];

	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-id",
						&id, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-id property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_id = id;

	/* regulator-type */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-type",
						&type, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-type property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_type = type;

	/* regulator-fake */
	ret = of_property_read_u32_array(np, "hisilicon,hisi-regulator-is-fake",
						&fake, 1);/*lint !e64*/
	if (ret) {
		dev_err(dev, "no hisilicon,hisi-regulator-is-fake property set\n");
		goto dt_parse_common_end;
	}
	sreg->regulator_fake = fake;

#ifdef CONFIG_HISI_NOC_HI3650_PLATFORM
	hisi_noc_dt_parse(sreg, pdev);
#endif
dt_parse_common_end:
	return ret;
}

/******************************ip voltage region ops*************************/
struct regulator_ops hisi_ip_atf_rops = {
	.is_enabled = hisi_ip_to_atf_is_enabled,
	.enable = hisi_ip_to_atf_enabled,
	.disable = hisi_ip_to_atf_disabled,
};
struct regulator_ops hisi_ip_lpm_rops = {
	.is_enabled = hisi_ip_to_lpm_is_enabled,
	.enable = hisi_ip_to_lpm_enabled,
	.disable = hisi_ip_to_lpm_disabled,
};

/******************************hisi_regulator_ip******************************/
static const struct hisi_regulator_ip hisi_regulator_ip_atf = {
	.rdesc = {
		.ops = &hisi_ip_atf_rops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	.dt_parse = hisi_dt_parse_ip_atf,
};

static const struct hisi_regulator_ip hisi_regulator_ip_lpm = {
	.rdesc = {
		.ops = &hisi_ip_lpm_rops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	.dt_parse = hisi_dt_parse_ip_lpm,
};

/*********************of_hisi_regulator_ip_match_tbl***************************/
static struct of_device_id of_hisi_regulator_ip_match_tbl[] = {
	{
		.compatible = "ip-regulator-atf",
		.data = &hisi_regulator_ip_atf,
	},
	{
		.compatible = "ip-regulator-lpm",
		.data = &hisi_regulator_ip_lpm,
	},
	{ /* end */ }
};
#ifdef CONFIG_HISI_PMIC_DEBUG
static int dbg_control_vcc_show(struct seq_file *s, void *data)
{
	pr_info("dbg_control_hisi_ip_show \n\r");
	return 0;
}
static ssize_t dbg_control_vcc_set_value(struct file *filp, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	char tmp[128] = {0};
	int index = 0;

	if (count >= 128 || !buffer) {
		pr_info("error! buffer size big than internal buffer\n");
		return -EFAULT;
	}

	if (copy_from_user(tmp, buffer, count)) {
		pr_info("error!\n");
		return -EFAULT;
	}

	if (sscanf(tmp, "%d", &index)) {
		if (index == 0) {

		} else if (index == 1) {

		} else {
			pr_info("ERRR~\n");
		}
	} else {
		pr_info("ERRR~\n");
	}

	*ppos += count;

	return count;
}

static int dbg_control_vcc_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return single_open(file, dbg_control_vcc_show, &inode->i_private);
}

static const struct file_operations set_control_vcc_fops = {
	.open		= dbg_control_vcc_open,
	.read		= seq_read,
	.write		= dbg_control_vcc_set_value,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int hisi_regulator_ip_probe(struct platform_device *pdev)
{
	struct device *dev = NULL;
	struct device_node *np = NULL;
	struct regulator_desc *rdesc;
	struct regulator_dev *rdev;
	struct hisi_regulator_ip *sreg = NULL;
	struct regulator_init_data *initdata;
	struct regulator_config config = { };
	const struct of_device_id *match;
	const struct hisi_regulator_ip *temp = NULL;
	int ret = 0;
	static int regulator_flag;
	const char *supplyname = NULL;
#ifdef CONFIG_HISI_PMIC_DEBUG
	struct dentry *d;
#endif

	if (pdev == NULL) {
		pr_err("[%s]regulator get  platform device para is err!\n", __func__);
		return -EINVAL;
	}

	dev = &pdev->dev;
	np = dev->of_node;

	/* to check which type of regulator this is */
	match = of_match_device(of_hisi_regulator_ip_match_tbl, &pdev->dev);
	if (NULL == match) {
		pr_err("get hi6421 regulator fail!\n\r");
		return -EINVAL;
	}

	temp = match->data;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 13, 0))
	initdata = of_get_regulator_init_data(dev, np, NULL);
#else
	initdata = of_get_regulator_init_data(dev, np);
#endif
	if (NULL == initdata) {
		pr_err("get regulator init data error !\n");
		return -EINVAL;
	}

	sreg = kmemdup(temp, sizeof(*sreg), GFP_KERNEL);
	if (!sreg)
		return -ENOMEM;

	sreg->name = initdata->constraints.name;
	rdesc = &sreg->rdesc;
	rdesc->name = sreg->name;

	supplyname = of_get_property(np, "hisilicon,supply_name", NULL);
	if (supplyname != NULL) {
		initdata->supply_regulator = supplyname;
	}

	mutex_init(&(sreg->regulator_lock));
	/* to parse device tree data for regulator specific */
	ret = sreg->dt_parse(sreg, pdev);
	if (ret) {
		dev_err(dev, "device tree parameter parse error!\n");
		goto hisi_ip_probe_end;
	}

	config.dev = &pdev->dev;
	config.init_data = initdata;
	config.driver_data = sreg;
	config.of_node = pdev->dev.of_node;
	sreg->np = np;
	sreg->regulator_enalbe_flag = 0;
	np = NULL;
	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np)
		pr_err("[%s] node doesn't have sysctrl node!\n", __func__);
	else
		sreg->sctrl_reg = of_iomap(np, 0);
	if (!sreg->sctrl_reg)
		pr_err("[%s] sysctrl iomap fail!\n", __func__);

	/* register regulator */
	rdev = regulator_register(rdesc, &config);
	if (IS_ERR(rdev)) {
		dev_err(dev, "failed to register %s\n",
			rdesc->name);
		ret = PTR_ERR(rdev);
		goto hisi_ip_probe_end;
	}

	if (regulator_flag == 0) {
#ifdef CONFIG_HISI_PMIC_DEBUG
		d = debugfs_create_dir("hisi_ip_debugfs", NULL);
		if (!d) {
			dev_err(dev, "failed to create ip regulator debugfs dir !\n");
			ret = -ENOMEM;
			goto hwspin_lock_err1;
		}
		(void) debugfs_create_file("control_vcc", S_IRUSR, d, NULL, &set_control_vcc_fops);
#endif
		regulator_flag = 1;
	}

	platform_set_drvdata(pdev, rdev);

	return 0;
#ifdef CONFIG_HISI_PMIC_DEBUG
hwspin_lock_err1:
	regulator_unregister(rdev);
#endif
hisi_ip_probe_end:
	kfree(sreg);
	return ret;
}

static int hisi_regulator_ip_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);
	struct hisi_regulator_ip *sreg = rdev_get_drvdata(rdev);


	regulator_unregister(rdev);
	/* TODO: should i worry about that? devm_kzalloc */
	if (sreg->rdesc.volt_table)
		devm_kfree(&pdev->dev, (unsigned int *)sreg->rdesc.volt_table);

	kfree(sreg);
	return 0;
}

static struct platform_driver hisi_regulator_ip_driver = {
	.driver = {
		.name	= "hisi_regulator_ip",
		.owner	= THIS_MODULE,
		.of_match_table = of_hisi_regulator_ip_match_tbl,
	},
	.probe	= hisi_regulator_ip_probe,
	.remove	= hisi_regulator_ip_remove,
};

static int __init hisi_regulator_ip_init(void)
{
	return platform_driver_register(&hisi_regulator_ip_driver);
}
fs_initcall(hisi_regulator_ip_init);

static void __exit hisi_regulator_ip_exit(void)
{
	platform_driver_unregister(&hisi_regulator_ip_driver);
}
module_exit(hisi_regulator_ip_exit);

MODULE_DESCRIPTION("Hisi regulator ip driver");
MODULE_LICENSE("GPL v2");
