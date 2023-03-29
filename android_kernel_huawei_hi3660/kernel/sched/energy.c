/*
 * Obtain energy cost data from DT and populate relevant scheduler data
 * structures.
 *
 * Copyright (C) 2015 ARM Ltd.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define pr_fmt(fmt) "sched-energy: " fmt

#define DEBUG

#include <linux/gfp.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched_energy.h>
#include <linux/stddef.h>

struct sched_group_energy *sge_array[NR_CPUS][NR_SD_LEVELS];

static void free_resources(void)
{
	int cpu, sd_level;
	struct sched_group_energy *sge;

	for_each_possible_cpu(cpu) {
		for_each_possible_sd_level(sd_level) {
			sge = sge_array[cpu][sd_level];
			if (sge) {
				kfree(sge->cap_states);
				kfree(sge->idle_states);
				kfree(sge);
			}
		}
	}
}

#ifdef CONFIG_HISI_EAS_SCHED
static void dump_energy_model(void)
{
	int cpu, sd_level, idx;
	struct sched_group_energy *sge;

	for_each_possible_cpu(cpu) {
		for_each_possible_sd_level(sd_level) {
			sge = sge_array[cpu][sd_level];
			if (!sge)
				continue;

			pr_info("EAS: cpu %d sd_level = %d\n", cpu, sd_level);

			for (idx = 0; idx < sge->nr_cap_states; idx++) {
				pr_info("Busy state [%d] = c %lu p %lu i %lu\n", idx,
					sge->cap_states[idx].cap,
					sge->cap_states[idx].power,
					sge->cap_states[idx].idle_power);
			}

			for (idx = 0; idx < sge->nr_idle_states; idx++)
				pr_info("Idle state [%d] = p %lu\n", idx,
					sge->idle_states[idx].power);
		}
	}
}
#endif

void init_sched_energy_costs(void)
{
	struct device_node *cn, *cp;
	struct capacity_state *cap_states;
	struct idle_state *idle_states;
	struct sched_group_energy *sge;
	const struct property *prop;
	int sd_level, i, nstates, cpu;
	const __be32 *val;

	for_each_possible_cpu(cpu) {
		cn = of_get_cpu_node(cpu, NULL);
		if (!cn) {
			pr_warn("CPU device node missing for CPU %d\n", cpu);
			return;
		}

		if (!of_find_property(cn, "sched-energy-costs", NULL)) {
			pr_warn("CPU device node has no sched-energy-costs\n");
			return;
		}

		for_each_possible_sd_level(sd_level) {
			cp = of_parse_phandle(cn, "sched-energy-costs", sd_level);
			if (!cp)
				break;

			prop = of_find_property(cp, "busy-cost-data", NULL);
			if (!prop || !prop->value) {
				pr_warn("No busy-cost data, skipping sched_energy init\n");
				goto out;
			}

			sge = kcalloc(1, sizeof(struct sched_group_energy),
				      GFP_NOWAIT);

			nstates = (prop->length / sizeof(u32)) / 2;
			cap_states = kcalloc(nstates,
					     sizeof(struct capacity_state),
					     GFP_NOWAIT);

			for (i = 0, val = prop->value; i < nstates; i++) {
				cap_states[i].cap = be32_to_cpup(val++);
				cap_states[i].power = be32_to_cpup(val++);
			}

			sge->nr_cap_states = nstates;
			sge->cap_states = cap_states;

#ifdef CONFIG_HISI_EAS_SCHED
			prop = of_find_property(cp, "static-cost-data", NULL);
			if (!prop || !prop->value)
				goto skip_static_cost_data;

			nstates = (prop->length / sizeof(u32));
			if (nstates != sge->nr_cap_states) {
				pr_warn("The static-cost-data may have wrong length, skipping sched_energy init");
				goto out;
			}

			for (i = 0, val = prop->value; i < nstates; i++)
				cap_states[i].idle_power = be32_to_cpup(val++);
skip_static_cost_data:
#endif

			prop = of_find_property(cp, "idle-cost-data", NULL);
			if (!prop || !prop->value) {
				pr_warn("No idle-cost data, skipping sched_energy init\n");
				goto out;
			}

			nstates = (prop->length / sizeof(u32));
			idle_states = kcalloc(nstates,
					      sizeof(struct idle_state),
					      GFP_NOWAIT);

			for (i = 0, val = prop->value; i < nstates; i++)
				idle_states[i].power = be32_to_cpup(val++);

			sge->nr_idle_states = nstates;
			sge->idle_states = idle_states;

			sge_array[cpu][sd_level] = sge;
		}
	}

#ifdef CONFIG_HISI_EAS_SCHED
	dump_energy_model();
#endif

	pr_info("Sched-energy-costs installed from DT\n");
	return;

out:
	free_resources();
}
