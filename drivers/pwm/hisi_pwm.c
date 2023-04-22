/*
 * hisi_pwm.c
 *
 * The driver for hisi kirin soc pwm.
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/time64.h>
#include <linux/kernel.h>

#define PWM_CHANNEL_NUMBER 2
#define PWM_PRESCALER_DEFAULT 1
#define PWM_WITH_FLAGS_N_CELLS 3

#define PWM_REG_LOCK_VAL 0x1ACCE550
#define PWM_REG_UNLOCK_VAL 0x1ACCE551
#define PWM_ENABLE_CFG_VAL 0x1
#define PWM_DISABLE_CFG_VAL 0x0
#define PWM_SINGLE_EDGE_ON_VAL 0x2
#define PWM_SINGLE_EDGE_OFF_VAL 0x0
#define PWM_DOUBLE_EDGE_ON_VAL 0x3
#define PWM_DOUBLE_EDGE_OFF_VAL 0x1

#define PWM_PR_OFFSET 0
#define PWM_PR_WIDTH 16
#define PWM_ENABLE_OFFSET 0
#define PWM_ENABLE_WIDTH 1
#define PWM_CFG_OFFSET(c) ((c) * 2)
#define PWM_CFG_WIDTH 2
#define PWM_CHAN_EN_OFFSET(c) (PWM_CFG_OFFSET(c) + 1)

#define MAX_PWM_CLK_RATE 120000000
#define MAX_PRESCALER 16

enum hisi_pwm_mode {
	SINGLE_EDGE,
	DOUBLE_EDGE
};

struct hisi_pwm_regs {
	u32 lock;
	u32 ctl;
	u32 cfg;
	u32 cx_mr0_base;
	u32 cx_mr0_step;
	u32 cx_mr1_base;
	u32 cx_mr1_step;
	u32 cx_mr_base;
	u32 cx_mr_step;
	u32 cx_pha_base;
	u32 cx_pha_step;
	u32 pr0;
	u32 pr1;
};

struct hisi_pwm_chip;
struct hisi_pwm_data {
	struct hisi_pwm_regs regs;
	unsigned int pwm_num;

	void (*set_polarity)(const struct hisi_pwm_chip *hpc,
		const struct pwm_device *pwm, enum pwm_polarity polarity);
	enum pwm_polarity (*get_polarity)(const struct hisi_pwm_chip *hpc,
		const struct pwm_device *pwm);
};

struct hisi_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
	unsigned long clk_rate;
	bool clk_enabled;
	const struct hisi_pwm_data *data;
	bool support_polarity;
	u32 prescaler0;
	u32 prescaler1;
	enum hisi_pwm_mode pwm_mode;
	spinlock_t reg_sequence_lock;
};

static const struct hisi_pwm_data pwm_data_v1 = {
	.regs = {
		.lock = 0x0000,
		.ctl = 0x0004,
		.cfg = 0x0008,
		.cx_mr0_base = 0x0304,
		.cx_mr0_step = 0x100,
		.cx_mr1_base = 0x0308,
		.cx_mr1_step = 0x100,
		.cx_mr_base = 0x0300,
		.cx_mr_step = 0x100,
		.cx_pha_base = 0x0310,
		.cx_pha_step = 0x100,
		.pr0 = 0x0100,
		.pr1 = 0x0104,
	},
	.pwm_num = PWM_CHANNEL_NUMBER,
	.set_polarity = NULL,
	.get_polarity = NULL,
};

static inline struct hisi_pwm_chip *to_hisi_pwm_chip(const struct pwm_chip *c)
{
	return container_of(c, struct hisi_pwm_chip, chip);
}

static inline void write_reg(void *reg_addr, u32 reg_val)
{
	writel(reg_val, reg_addr);
}

static void write_partial_reg(void *reg, u32 offset, u32 width, u32 val)
{
	u32 mask;
	u32 tmp;

	if (width == 0)
		return;

	mask = GENMASK(offset + width - 1, offset);
	tmp = readl(reg);
	tmp &= ~mask;
	tmp |= ((val << offset) & mask);
	write_reg(reg, tmp);
}

static void hisi_pwm_prescaler_config(const struct hisi_pwm_chip *hpc)
{
	u32 pr0;
	u32 pr1;

	pr0 = (hpc->prescaler0 == 0) ? 0 : (hpc->prescaler0 - 1);
	pr1 = (hpc->prescaler1 == 0) ? 0 : (hpc->prescaler1 - 1);
	write_partial_reg(hpc->base + hpc->data->regs.pr0,
		PWM_PR_OFFSET, PWM_PR_WIDTH, pr0);
	write_partial_reg(hpc->base + hpc->data->regs.pr1,
		PWM_PR_OFFSET, PWM_PR_WIDTH, pr1);
}

static void _hisi_config_mrx(const struct hisi_pwm_chip *hpc,
	void *reg_addr, u32 val_ns, bool period)
{
	u64 dividend;
	u32 divisor;
	u32 reg_val;

	if ((hpc->clk_rate == 0) ||
		(hpc->prescaler0 == 0) ||
		(hpc->prescaler1 == 0)) {
		dev_err(hpc->chip.dev, "%s, param is 0\n", __func__);
		return;
	}
	if ((hpc->clk_rate > MAX_PWM_CLK_RATE) ||
		(val_ns > NSEC_PER_SEC) ||
		(hpc->prescaler0 > MAX_PRESCALER) ||
		(hpc->prescaler1 > MAX_PRESCALER)) {
		dev_err(hpc->chip.dev, "%s, param exceed limit\n", __func__);
		return;
	}

	dividend = hpc->clk_rate * val_ns;
	divisor = hpc->prescaler0 * hpc->prescaler1;
	divisor = (divisor == 0) ? 1 : divisor;
	dividend = DIV_ROUND_CLOSEST_ULL(dividend, divisor);
	dividend = DIV_ROUND_CLOSEST_ULL(dividend, NSEC_PER_SEC);
	if (dividend > (u64)U32_MAX) {
		dev_err(hpc->chip.dev, "%s, val exceed U32_MAX\n", __func__);
		return;
	}
	reg_val = (u32)dividend;

	switch (hpc->pwm_mode) {
	case SINGLE_EDGE:
		if (period) {
			if (reg_val > 0)
				reg_val -= 1;
		} else if (reg_val == 0) {
			dev_err(hpc->chip.dev,
				"%s, single edge, duty is 0\n", __func__);
			return;
		}
		break;
	case DOUBLE_EDGE:
		dev_err(hpc->chip.dev,
			"%s, double edge, not implemented\n", __func__);
		return;
	default:
		dev_err(hpc->chip.dev, "%s, unknown pwm_mode\n", __func__);
		return;
	}

	write_reg(reg_addr, reg_val);
}

static void hisi_channel_duty_config(const struct hisi_pwm_chip *hpc,
	u32 chan, u32 duty_ns)
{
	void *reg_addr = NULL;
	u32 offset;

	offset = hpc->data->regs.cx_mr0_base +
		chan * hpc->data->regs.cx_mr0_step;
	reg_addr = hpc->base + offset;
	_hisi_config_mrx(hpc, reg_addr, duty_ns, false);
}

static void hisi_channel_period_config(const struct hisi_pwm_chip *hpc,
	u32 chan, u32 period_ns)
{
	void *reg_addr = NULL;
	u32 offset;

	offset = hpc->data->regs.cx_mr_base +
		chan * hpc->data->regs.cx_mr_step;
	reg_addr = hpc->base + offset;
	_hisi_config_mrx(hpc, reg_addr, period_ns, true);
}

static void hisi_channel_phase_config(const struct hisi_pwm_chip *hpc,
	u32 chan, u32 phase_val)
{
	void *reg_addr = NULL;
	u32 offset;

	offset = hpc->data->regs.cx_pha_base +
		chan * hpc->data->regs.cx_pha_step;
	reg_addr = hpc->base + offset;
	write_reg(reg_addr, phase_val);
}

static void hisi_channel_polarity_config(const struct hisi_pwm_chip *hpc,
	const struct pwm_device *pwm, enum pwm_polarity polarity)
{
	if (!hpc->support_polarity)
		return;
	if (hpc->data->set_polarity == NULL)
		return;
	hpc->data->set_polarity(hpc, pwm, polarity);
}

static void hisi_channel_output_config(const struct hisi_pwm_chip *hpc,
	u32 channel, bool on)
{
	u32 output_cfg;

	switch (hpc->pwm_mode) {
	case SINGLE_EDGE:
		if (on)
			output_cfg = PWM_SINGLE_EDGE_ON_VAL;
		else
			output_cfg = PWM_SINGLE_EDGE_OFF_VAL;
		break;
	case DOUBLE_EDGE:
		if (on)
			output_cfg = PWM_DOUBLE_EDGE_ON_VAL;
		else
			output_cfg = PWM_DOUBLE_EDGE_OFF_VAL;
		break;
	default:
		dev_err(hpc->chip.dev, "%s, unknown pwm_mode\n", __func__);
		return;
	}
	write_partial_reg(hpc->base + hpc->data->regs.cfg,
		PWM_CFG_OFFSET(channel), PWM_CFG_WIDTH,
		output_cfg);
}

static inline void hisi_pwm_reg_unlock(const struct hisi_pwm_chip *hpc)
{
	write_reg(hpc->base + hpc->data->regs.lock, PWM_REG_UNLOCK_VAL);
}

static inline void hisi_pwm_reg_lock(const struct hisi_pwm_chip *hpc)
{
	write_reg(hpc->base + hpc->data->regs.lock, PWM_REG_LOCK_VAL);
}

static inline void hisi_pwm_disable(const struct hisi_pwm_chip *hpc)
{
	write_partial_reg(hpc->base + hpc->data->regs.ctl,
		PWM_ENABLE_OFFSET, PWM_ENABLE_WIDTH, PWM_DISABLE_CFG_VAL);
}

static inline void hisi_pwm_enable(const struct hisi_pwm_chip *hpc)
{
	write_partial_reg(hpc->base + hpc->data->regs.ctl,
		PWM_ENABLE_OFFSET, PWM_ENABLE_WIDTH, PWM_ENABLE_CFG_VAL);
}

static void hisi_pwm_config(const struct hisi_pwm_chip *hpc,
	const struct pwm_device *pwm,
	u32 period_ns, u32 duty_ns, enum pwm_polarity polarity)
{
	hisi_pwm_prescaler_config(hpc);
	hisi_channel_period_config(hpc, pwm->hwpwm, period_ns);
	hisi_channel_duty_config(hpc, pwm->hwpwm, duty_ns);
	hisi_channel_phase_config(hpc, pwm->hwpwm, 0);
	hisi_channel_polarity_config(hpc, pwm, polarity);
}

/*
 * Helper function to manage clk enable state.
 * 1.Without clk enabled, the pwm reg cannot be read.
 * 2.pwm_apply maybe called with enable and disable state not the same counts.
 *
 * @param hpc driver data.
 * @return int 0 ok, otherwise fail.
 */
static int hisi_pwm_clk_enable(struct hisi_pwm_chip *hpc)
{
	int ret;

	if (hpc->clk_enabled)
		return 0;
	ret = clk_enable(hpc->clk);
	if (ret != 0) {
		dev_err(hpc->chip.dev, "%s, clk_enable fail\n", __func__);
		return ret;
	}
	hpc->clk_enabled = true;
	return 0;
}

static void hisi_pwm_clk_disable(struct hisi_pwm_chip *hpc)
{
	if (!hpc->clk_enabled)
		return;
	clk_disable(hpc->clk);
	hpc->clk_enabled = false;
}

static bool hisi_channel_enabled(const struct hisi_pwm_chip *hpc,
	const struct pwm_device *pwm)
{
	u32 pwm_ctl;
	u32 pwm_cfg;
	u32 offset;
	u32 mask;

	mask = GENMASK(PWM_ENABLE_OFFSET + PWM_ENABLE_WIDTH - 1,
		PWM_ENABLE_OFFSET);
	pwm_ctl = readl(hpc->base + hpc->data->regs.ctl);
	pwm_ctl &= mask;
	pwm_ctl >>= PWM_ENABLE_OFFSET;

	offset = PWM_CFG_OFFSET(pwm->hwpwm);
	mask = GENMASK(offset + PWM_CFG_WIDTH - 1, offset);
	pwm_cfg = readl(hpc->base + hpc->data->regs.cfg);
	pwm_cfg &= mask;
	if (pwm_ctl == PWM_ENABLE_CFG_VAL) {
		if (pwm_cfg & BIT(PWM_CHAN_EN_OFFSET(pwm->hwpwm)))
			return true;
	}

	return false;
}

static bool hisi_all_other_channels_disabled(const struct hisi_pwm_chip *hpc,
	u32 cur_chan)
{
	u32 i;

	for (i = 0; i < hpc->chip.npwm; i++) {
		if ((i != cur_chan) && pwm_is_enabled(hpc->chip.pwms + i))
			return false;
	}
	return true;
}

static void _hisi_calc_state(const struct hisi_pwm_chip *hpc,
	const struct pwm_device *pwm, struct pwm_state *state)
{
	u32 reg_offset;
	u32 reg_val;
	u64 tmp;
	void *reg_period = NULL;
	void *reg_duty = NULL;

	if ((hpc->clk_rate == 0) ||
		(hpc->prescaler0 == 0) ||
		(hpc->prescaler1 == 0)) {
		dev_err(hpc->chip.dev, "%s, param is 0\n", __func__);
		return;
	}
	if ((hpc->clk_rate > MAX_PWM_CLK_RATE) ||
		(hpc->prescaler0 > MAX_PRESCALER) ||
		(hpc->prescaler1 > MAX_PRESCALER)) {
		dev_err(hpc->chip.dev, "%s, param exceed limit\n", __func__);
		return;
	}

	reg_offset = hpc->data->regs.cx_mr_base +
		pwm->hwpwm * hpc->data->regs.cx_mr_step;
	reg_period = hpc->base + reg_offset;
	reg_val = readl(reg_period);
	tmp = ((u64)reg_val + 1) * (u64)hpc->prescaler0 *
		(u64)hpc->prescaler1 * NSEC_PER_SEC;
	state->period = DIV_ROUND_CLOSEST_ULL(tmp, hpc->clk_rate);

	reg_offset = hpc->data->regs.cx_mr0_base +
		pwm->hwpwm * hpc->data->regs.cx_mr0_step;
	reg_duty = hpc->base + reg_offset;
	reg_val = readl(reg_duty);
	tmp = (u64)reg_val * (u64)hpc->prescaler0 *
		(u64)hpc->prescaler1 * NSEC_PER_SEC;
	state->duty_cycle = DIV_ROUND_CLOSEST_ULL(tmp, hpc->clk_rate);

	state->polarity = PWM_POLARITY_NORMAL;
	if (hpc->support_polarity) {
		if (hpc->data->get_polarity != NULL)
			state->polarity = hpc->data->get_polarity(hpc, pwm);
	}

	state->enabled = hisi_channel_enabled(hpc, pwm);
}

static void hisi_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
	struct pwm_state *state)
{
	struct hisi_pwm_chip *hpc = NULL;

	if ((chip == NULL) || (pwm == NULL) || (state == NULL))
		return;

	hpc = to_hisi_pwm_chip(chip);
	spin_lock(&hpc->reg_sequence_lock);

	if (clk_enable(hpc->clk) != 0) {
		spin_unlock(&hpc->reg_sequence_lock);
		dev_err(chip->dev, "%s, clk_enable fail\n", __func__);
		return;
	}

	_hisi_calc_state(hpc, pwm, state);

	clk_disable(hpc->clk);
	spin_unlock(&hpc->reg_sequence_lock);
}

static int hisi_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
	struct pwm_state *state)
{
	struct hisi_pwm_chip *hpc = NULL;
	struct pwm_state cur_state;
	bool enabled = false;

	if ((chip == NULL) || (pwm == NULL) || (state == NULL))
		return -EINVAL;

	hpc = to_hisi_pwm_chip(chip);
	spin_lock(&hpc->reg_sequence_lock);

	if (hisi_pwm_clk_enable(hpc) != 0) {
		spin_unlock(&hpc->reg_sequence_lock);
		return -ENODEV;
	}

	hisi_pwm_reg_unlock(hpc);

	pwm_get_state(pwm, &cur_state);
	enabled = cur_state.enabled;

	// dynamically config duty
	if (enabled && state->duty_cycle != cur_state.duty_cycle &&
		state->period == cur_state.period &&
		state->polarity == cur_state.polarity) {
		hisi_channel_duty_config(hpc, pwm->hwpwm, state->duty_cycle);
		hisi_pwm_reg_lock(hpc);
		spin_unlock(&hpc->reg_sequence_lock);
		return 0;
	}

	if (state->enabled) {
		hisi_pwm_disable(hpc);
		hisi_pwm_config(hpc, pwm,
			state->period, state->duty_cycle, state->polarity);
		hisi_channel_output_config(hpc, pwm->hwpwm, true);
		hisi_pwm_enable(hpc);
		hisi_pwm_reg_lock(hpc);
	} else {
		bool disable_all = false;

		hisi_channel_output_config(hpc, pwm->hwpwm, false);
		if (hisi_all_other_channels_disabled(hpc, pwm->hwpwm)) {
			hisi_pwm_disable(hpc);
			disable_all = true;
		}
		hisi_pwm_reg_lock(hpc);
		if (disable_all)
			hisi_pwm_clk_disable(hpc);
	}

	spin_unlock(&hpc->reg_sequence_lock);

	// read from hw
	hisi_pwm_get_state(chip, pwm, state);
	return 0;
}

static const struct pwm_ops hisi_pwm_ops = {
	.get_state = hisi_pwm_get_state,
	.apply = hisi_pwm_apply,
	.owner = THIS_MODULE,
};

static const struct of_device_id hisi_pwm_dt_ids[] = {
	{ .compatible = "hisilicon,pwm", .data = &pwm_data_v1 },
	{},
};

static int hisi_pwm_parse_dt(struct hisi_pwm_chip *hpc,
	const struct device_node *np)
{
	int ret;

	hpc->support_polarity = of_property_read_bool(np, "support_polarity");
	ret = of_property_read_u32(np, "pre_freq_divisor0", &hpc->prescaler0);
	if (ret != 0)
		hpc->prescaler0 = PWM_PRESCALER_DEFAULT;
	ret = of_property_read_u32(np, "pre_freq_divisor1", &hpc->prescaler1);
	if (ret != 0)
		hpc->prescaler1 = PWM_PRESCALER_DEFAULT;
	if (of_property_read_bool(np, "double_edge_mode"))
		hpc->pwm_mode = DOUBLE_EDGE;
	else
		hpc->pwm_mode = SINGLE_EDGE;
	return 0;
}

static int hisi_add_pwm_chip(struct hisi_pwm_chip *hpc, struct device *pdev)
{
	int ret;

	hpc->chip.dev = pdev;
	hpc->chip.ops = &hisi_pwm_ops;
	hpc->chip.base = -1;
	hpc->chip.npwm = hpc->data->pwm_num;
	hpc->chip.can_sleep = false;

	if (hpc->support_polarity) {
		hpc->chip.of_xlate = of_pwm_xlate_with_flags;
		hpc->chip.of_pwm_n_cells = PWM_WITH_FLAGS_N_CELLS;
	}

	ret = pwmchip_add(&hpc->chip);
	if (ret != 0) {
		dev_err(pdev, "%s, pwmchip_add failed\n", __func__);
		return ret;
	}
	return 0;
}

static int hisi_clk_prepare(struct hisi_pwm_chip *hpc,
	struct platform_device *pdev)
{
	int ret;

	hpc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(hpc->clk)) {
		dev_err(&pdev->dev, "%s, clk_get fail\n", __func__);
		return PTR_ERR(hpc->clk);
	}

	ret = clk_prepare_enable(hpc->clk);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"%s, clk_prepare_enable fail\n", __func__);
		return ret;
	}
	hpc->clk_rate = clk_get_rate(hpc->clk);
	clk_disable(hpc->clk);
	hpc->clk_enabled = false;
	if (hpc->clk_rate == 0) {
		clk_unprepare(hpc->clk);
		dev_err(&pdev->dev, "%s, clk_rate is 0\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int hisi_pwm_probe(struct platform_device *pdev)
{
	const struct of_device_id *device_id = NULL;
	struct hisi_pwm_chip *hpc = NULL;
	struct resource *r = NULL;
	int ret;

	if (pdev == NULL)
		return -EINVAL;

	device_id = of_match_device(hisi_pwm_dt_ids, &pdev->dev);
	if (device_id == NULL) {
		dev_err(&pdev->dev, "%s, no matched device\n", __func__);
		return -EINVAL;
	}

	hpc = devm_kzalloc(&pdev->dev, sizeof(*hpc), GFP_KERNEL);
	if (hpc == NULL)
		return -ENOMEM;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hpc->base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(hpc->base)) {
		dev_err(&pdev->dev, "%s, ioremap_resource fail\n", __func__);
		return PTR_ERR(hpc->base);
	}

	ret = hisi_clk_prepare(hpc, pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "%s, hisi_clk_prepare fail\n", __func__);
		return ret;
	}

	hpc->data = device_id->data;

	ret = hisi_pwm_parse_dt(hpc, pdev->dev.of_node);
	if (ret != 0) {
		dev_err(&pdev->dev, "%s, parse_dt fail\n", __func__);
		goto fail_end;
	}

	spin_lock_init(&hpc->reg_sequence_lock);
	ret = hisi_add_pwm_chip(hpc, &pdev->dev);
	if (ret != 0)
		goto fail_end;
	platform_set_drvdata(pdev, hpc);
	dev_info(&pdev->dev, "%s, clk_rate:%lu\n", __func__, hpc->clk_rate);
	return 0;

fail_end:
	clk_unprepare(hpc->clk);
	return ret;
}

static int hisi_pwm_remove(struct platform_device *pdev)
{
	int ret;
	struct hisi_pwm_chip *hpc = NULL;

	if (pdev == NULL)
		return -EINVAL;

	hpc = platform_get_drvdata(pdev);
	if (hpc == NULL) {
		dev_err(&pdev->dev, "%s, hpc is NULL\n", __func__);
		return -EINVAL;
	}

	hisi_pwm_clk_disable(hpc);
	clk_unprepare(hpc->clk);

	ret = pwmchip_remove(&hpc->chip);
	if (ret != 0) {
		dev_err(&pdev->dev, "%s, pwmchip_remove fail\n", __func__);
		return ret;
	}
	return 0;
}

static struct platform_driver hisi_pwm_driver = {
	.driver = {
		.name = "hisi-pwm",
		.of_match_table = hisi_pwm_dt_ids,
	},
	.probe = hisi_pwm_probe,
	.remove = hisi_pwm_remove,
};
module_platform_driver(hisi_pwm_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("hisi kirin soc pwm driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
