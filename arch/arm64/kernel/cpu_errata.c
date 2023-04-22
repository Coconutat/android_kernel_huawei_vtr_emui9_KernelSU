/*
 * Contains CPU specific errata definitions
 *
 * Copyright (C) 2014 ARM Ltd.
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

#include <linux/arm-smccc.h>
#include <linux/psci.h>
#include <linux/types.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/cpufeature.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/perf/arm_pmu.h>

#ifdef CONFIG_HISI_HARDEN_BRANCH_PREDICTOR
struct wa2_pmu_monitor {
	struct perf_event *pevent;
	bool monitor_started;
};

DEFINE_PER_CPU(struct wa2_pmu_monitor, wa2_monitor);
DEFINE_PER_CPU_READ_MOSTLY(u64, pmu_counter);
DEFINE_PER_CPU_READ_MOSTLY(u64, v2_apply);

bool need_to_apply(int cpu)
{
	struct wa2_pmu_monitor* monitor = &per_cpu(wa2_monitor, cpu);
	u64 need_apply = 0;

	if (!monitor->monitor_started || monitor->pevent == NULL)
		return true;

	need_apply = per_cpu(v2_apply, cpu);
	if (need_apply) {
		per_cpu(v2_apply, cpu) = 0;
		return true;
	} else {
		return false;
	}
}

static struct perf_event_attr *alloc_attr(void)
{
	struct perf_event_attr *attr;

	attr = kzalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
	if (!attr)
		return attr;

	attr->type = PERF_TYPE_RAW;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;
	attr->exclude_idle = 1;

	return attr;
}

/* ignore debug code */
int get_wa2_monitor_status(void)
{
	struct wa2_pmu_monitor *monitor;
	unsigned long stat = 0;
	int cpu;
	u64 counter, need_apply;

	for_each_possible_cpu(cpu) {
		need_apply = per_cpu(v2_apply, cpu);
		monitor = &per_cpu(wa2_monitor, cpu);
		if (monitor->monitor_started) {
			stat |= BIT(cpu);
			counter = per_cpu(pmu_counter, cpu);
			pr_err("cpu=%d wa2_monitor_enable, counter=%llu, need-apply=%llu\n", cpu, counter, need_apply);
		} else {
			pr_err("cpu=%d,wa2_monitor_disable, need_apply=%llu\n", cpu, need_apply);
		}
		per_cpu(v2_apply, cpu) = 0;
	}

	return stat;
}

extern void hisi_get_slow_cpus(struct cpumask * cpumask);
extern u32 armv8pmu_get_counter(struct perf_event *event);
static int __init wa2_monitor_init(void)
{
	int cpu;
	struct wa2_pmu_monitor *monitor;
	struct perf_event *pevent;
	struct perf_event_attr *attr;
	struct cpumask slow_cpus;

	attr = alloc_attr();
	if (!attr)
		return -ENOMEM;

	attr->config = 0x11c;

	hisi_get_slow_cpus(&slow_cpus);

	for_each_possible_cpu(cpu) {
		monitor = &per_cpu(wa2_monitor, cpu);

		/* littlecores doesn't need to enable this PMU event */
		if (cpumask_test_cpu(cpu, &slow_cpus)) {
			monitor->monitor_started = false;
			monitor->pevent = NULL;

			per_cpu(pmu_counter, cpu) = 0;
			continue;
		}

		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent)) {
			monitor->monitor_started = false;
			monitor->pevent = NULL;
			per_cpu(pmu_counter, cpu) = 0;
			pr_err("fail to create wa2 pmu counter for cpu%d\n", cpu);
		} else {
			perf_event_enable(pevent);
			per_cpu(pmu_counter, cpu) = armv8pmu_get_counter(pevent);

			monitor->pevent = pevent;
			monitor->monitor_started = true;
			pr_info("success to create wa2 pmu counter for cpu%d\n", cpu);
		}
	}

	kfree(attr);

	return 0;
}

static void __exit wa2_monitor_exit(void)
{
	int cpu;
	struct wa2_pmu_monitor *monitor;

	for_each_possible_cpu(cpu) {
		monitor = &per_cpu(wa2_monitor, cpu);
		if (!monitor->monitor_started)
			continue;

		monitor->monitor_started = false;
		perf_event_release_kernel(monitor->pevent);
		monitor->pevent = NULL;
	}
}

late_initcall(wa2_monitor_init);
module_exit(wa2_monitor_exit);
#endif

static bool __maybe_unused
is_affected_midr_range(const struct arm64_cpu_capabilities *entry, int scope)
{
	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return MIDR_IS_CPU_MODEL_RANGE(read_cpuid_id(), entry->midr_model,
				       entry->midr_range_min,
				       entry->midr_range_max);
}

static bool
has_mismatched_cache_line_size(const struct arm64_cpu_capabilities *entry,
				int scope)
{
	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return (read_cpuid_cachetype() & arm64_ftr_reg_ctrel0.strict_mask) !=
		(arm64_ftr_reg_ctrel0.sys_val & arm64_ftr_reg_ctrel0.strict_mask);
}

static int cpu_enable_trap_ctr_access(void *__unused)
{
	/* Clear SCTLR_EL1.UCT */
	config_sctlr_el1(SCTLR_EL1_UCT, 0);
	return 0;
}

#ifdef CONFIG_HARDEN_BRANCH_PREDICTOR
#include <asm/mmu_context.h>
#include <asm/cacheflush.h>

DEFINE_PER_CPU_READ_MOSTLY(struct bp_hardening_data, bp_hardening_data);

#ifdef CONFIG_KVM
extern char __smccc_workaround_1_smc_start[];
extern char __smccc_workaround_1_smc_end[];
extern char __smccc_workaround_1_hvc_start[];
extern char __smccc_workaround_1_hvc_end[];

static void __copy_hyp_vect_bpi(int slot, const char *hyp_vecs_start,
				const char *hyp_vecs_end)
{
	void *dst = __bp_harden_hyp_vecs_start + slot * SZ_2K;
	int i;

	for (i = 0; i < SZ_2K; i += 0x80)
		memcpy(dst + i, hyp_vecs_start, hyp_vecs_end - hyp_vecs_start);

	flush_icache_range((uintptr_t)dst, (uintptr_t)dst + SZ_2K);
}

static void __install_bp_hardening_cb(bp_hardening_cb_t fn,
				      const char *hyp_vecs_start,
				      const char *hyp_vecs_end)
{
	static int last_slot = -1;
	static DEFINE_SPINLOCK(bp_lock);
	int cpu, slot = -1;

	spin_lock(&bp_lock);
	for_each_possible_cpu(cpu) {
		if (per_cpu(bp_hardening_data.fn, cpu) == fn) {
			slot = per_cpu(bp_hardening_data.hyp_vectors_slot, cpu);
			break;
		}
	}

	if (slot == -1) {
		last_slot++;
		BUG_ON(((__bp_harden_hyp_vecs_end - __bp_harden_hyp_vecs_start)
			/ SZ_2K) <= last_slot);
		slot = last_slot;
		__copy_hyp_vect_bpi(slot, hyp_vecs_start, hyp_vecs_end);
	}

	__this_cpu_write(bp_hardening_data.hyp_vectors_slot, slot);
	__this_cpu_write(bp_hardening_data.fn, fn);
	spin_unlock(&bp_lock);
}
#else
#define __smccc_workaround_1_smc_start		NULL
#define __smccc_workaround_1_smc_end		NULL
#define __smccc_workaround_1_hvc_start		NULL
#define __smccc_workaround_1_hvc_end		NULL

static void __install_bp_hardening_cb(bp_hardening_cb_t fn,
				      const char *hyp_vecs_start,
				      const char *hyp_vecs_end)
{
	__this_cpu_write(bp_hardening_data.fn, fn);
}
#endif	/* CONFIG_KVM */

static void  install_bp_hardening_cb(const struct arm64_cpu_capabilities *entry,
				     bp_hardening_cb_t fn,
				     const char *hyp_vecs_start,
				     const char *hyp_vecs_end)
{
	u64 pfr0;

	if (!entry->matches(entry, SCOPE_LOCAL_CPU))
		return;

	pfr0 = read_cpuid(ID_AA64PFR0_EL1);
	if (cpuid_feature_extract_unsigned_field(pfr0, ID_AA64PFR0_CSV2_SHIFT))
		return;

	__install_bp_hardening_cb(fn, hyp_vecs_start, hyp_vecs_end);
}

#include <uapi/linux/psci.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>

static void call_smc_arch_workaround_1(void)
{
	arm_smccc_1_1_smc(ARM_SMCCC_ARCH_WORKAROUND_1, NULL);
}

#if 0
static void call_hvc_arch_workaround_1(void)
{
	arm_smccc_1_1_hvc(ARM_SMCCC_ARCH_WORKAROUND_1, NULL);
}
#endif

static int enable_smccc_arch_workaround_1(void *data)
{
	const struct arm64_cpu_capabilities *entry = data;
	bp_hardening_cb_t cb;
	void *smccc_start, *smccc_end;
#if 0
	struct arm_smccc_res res;
#endif

	unsigned long part_num = read_cpuid_part_number();

	if ((part_num != ARM_CPU_PART_CORTEX_A72)
			&& (part_num != ARM_CPU_PART_CORTEX_A73)
			&& (part_num != ARM_CPU_PART_ENYO))
		return 0;

	if (!entry->matches(entry, SCOPE_LOCAL_CPU))
		return 0;

	cb = call_smc_arch_workaround_1;
	smccc_start = __smccc_workaround_1_smc_start;
	smccc_end = __smccc_workaround_1_smc_end;

#if 0
	if (psci_ops.smccc_version == SMCCC_VERSION_1_0)
		return 0;

	switch (psci_ops.conduit) {
	case PSCI_CONDUIT_HVC:
		arm_smccc_1_1_hvc(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
				  ARM_SMCCC_ARCH_WORKAROUND_1, &res);
		if ((int)res.a0 < 0)
			return 0;
		cb = call_hvc_arch_workaround_1;
		smccc_start = __smccc_workaround_1_hvc_start;
		smccc_end = __smccc_workaround_1_hvc_end;
		break;

	case PSCI_CONDUIT_SMC:
		arm_smccc_1_1_smc(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
				  ARM_SMCCC_ARCH_WORKAROUND_1, &res);
		if ((int)res.a0 < 0)
			return 0;
		cb = call_smc_arch_workaround_1;
		smccc_start = __smccc_workaround_1_smc_start;
		smccc_end = __smccc_workaround_1_smc_end;
		break;

	default:
		return 0;
	}
#endif

	install_bp_hardening_cb(entry, cb, smccc_start, smccc_end);

	return 0;
}
#endif	/* CONFIG_HARDEN_BRANCH_PREDICTOR */

#ifdef CONFIG_ARM64_SSBD
DEFINE_PER_CPU_READ_MOSTLY(u64, arm64_ssbd_callback_required);

int ssbd_state __read_mostly = ARM64_SSBD_KERNEL;

static const struct ssbd_options {
	const char	*str;
	int		state;
} ssbd_options[] = {
	{ "force-on",	ARM64_SSBD_FORCE_ENABLE, },
	{ "force-off",	ARM64_SSBD_FORCE_DISABLE, },
	{ "kernel",	ARM64_SSBD_KERNEL, },
};

static int __init ssbd_cfg(char *buf)
{
	int i;

	if (!buf || !buf[0])
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(ssbd_options); i++) {
		int len = strlen(ssbd_options[i].str);

		if (strncmp(buf, ssbd_options[i].str, len))
			continue;

		ssbd_state = ssbd_options[i].state;
		return 0;
	}

	return -EINVAL;
}
early_param("ssbd", ssbd_cfg);

void __init arm64_update_smccc_conduit(struct alt_instr *alt,
				       __le32 *origptr, __le32 *updptr,
				       int nr_inst)
{
	u32 insn;

	BUG_ON(nr_inst != 1);

#if 0
	switch (psci_ops.conduit) {
	case PSCI_CONDUIT_HVC:
		insn = aarch64_insn_get_hvc_value();
		break;
	case PSCI_CONDUIT_SMC:
		insn = aarch64_insn_get_smc_value();
		break;
	default:
		return;
	}
#endif
	insn = aarch64_insn_get_smc_value();

	*updptr = cpu_to_le32(insn);
}

void __init arm64_enable_wa2_handling(struct alt_instr *alt,
				      __le32 *origptr, __le32 *updptr,
				      int nr_inst)
{
	BUG_ON(nr_inst != 1);
	/*
	 * Only allow mitigation on EL1 entry/exit and guest
	 * ARCH_WORKAROUND_2 handling if the SSBD state allows it to
	 * be flipped.
	 */
	if (arm64_get_ssbd_state() == ARM64_SSBD_KERNEL)
		*updptr = cpu_to_le32(aarch64_insn_gen_nop());
}

void arm64_set_ssbd_mitigation(bool state)
{
#if 0
	switch (psci_ops.conduit) {
	case PSCI_CONDUIT_HVC:
		arm_smccc_1_1_hvc(ARM_SMCCC_ARCH_WORKAROUND_2, state, NULL);
		break;

	case PSCI_CONDUIT_SMC:
		arm_smccc_1_1_smc(ARM_SMCCC_ARCH_WORKAROUND_2, state, NULL);
		break;

	default:
		WARN_ON_ONCE(1);
		break;
	}
#endif
	arm_smccc_1_1_smc(ARM_SMCCC_ARCH_WORKAROUND_2, state, NULL);
}

static bool has_ssbd_mitigation(const struct arm64_cpu_capabilities *entry,
				    int scope)
{
	struct arm_smccc_res res;
	bool required = true;
	s32 val;

	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());

#if 0
	if (psci_ops.smccc_version == SMCCC_VERSION_1_0)
		ssbd_state = ARM64_SSBD_UNKNOWN;
		return false;
	/*
	 * The probe function return value is either negative
	 * (unsupported or mitigated), positive (unaffected), or zero
	 * (requires mitigation). We only need to do anything in the
	 * last case.
	 */

	switch (psci_ops.conduit) {
	case PSCI_CONDUIT_HVC:
		arm_smccc_1_1_hvc(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
				  ARM_SMCCC_ARCH_WORKAROUND_2, &res);
		break;

	case PSCI_CONDUIT_SMC:
		arm_smccc_1_1_smc(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
				  ARM_SMCCC_ARCH_WORKAROUND_2, &res);
		break;

	default:
		ssbd_state = ARM64_SSBD_UNKNOWN;
		return false;
	}
#endif
	arm_smccc_1_1_smc(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
			  ARM_SMCCC_ARCH_WORKAROUND_2, &res);

	val = (s32)res.a0;

	switch (val) {
	case SMCCC_RET_NOT_SUPPORTED:
		pr_info("%s mitigation not supported\n", entry->desc);
		ssbd_state = ARM64_SSBD_UNKNOWN;
		return false;

	case SMCCC_RET_NOT_REQUIRED:
		pr_info("%s mitigation not required,%d\n", entry->desc, val);
		ssbd_state = ARM64_SSBD_MITIGATED;
		return false;

	case SMCCC_RET_SUCCESS:
		pr_info("%s mitigation capable\n", entry->desc);
		required = true;
		break;

	case 1:	/* Mitigation not required on this CPU */
		pr_info("%s mitigation not required,%d\n", entry->desc, val);
		required = false;
		break;

	default:
		WARN_ON(1);
		return false;
	}

	switch (ssbd_state) {
	case ARM64_SSBD_FORCE_DISABLE:
		pr_info("%s disabled from command-line\n", entry->desc);
		arm64_set_ssbd_mitigation(false);
		required = false;
		break;

	case ARM64_SSBD_KERNEL:
		if (required) {
			pr_info("%s kernel enabled\n", entry->desc);
			__this_cpu_write(arm64_ssbd_callback_required, 1);
			arm64_set_ssbd_mitigation(true);
		}
		break;

	case ARM64_SSBD_FORCE_ENABLE:
		pr_info("%s forced from command-line\n", entry->desc);
		arm64_set_ssbd_mitigation(true);
		required = true;
		break;

	default:
		WARN_ON(1);
		break;
	}

	return required;
}
#endif	/* CONFIG_ARM64_SSBD */

#define MIDR_RANGE(model, min, max) \
	.def_scope = SCOPE_LOCAL_CPU, \
	.matches = is_affected_midr_range, \
	.midr_model = model, \
	.midr_range_min = min, \
	.midr_range_max = max

#define MIDR_ALL_VERSIONS(model) \
	.def_scope = SCOPE_LOCAL_CPU, \
	.matches = is_affected_midr_range, \
	.midr_model = model, \
	.midr_range_min = 0, \
	.midr_range_max = (MIDR_VARIANT_MASK | MIDR_REVISION_MASK)

const struct arm64_cpu_capabilities arm64_errata[] = {
#if	defined(CONFIG_ARM64_ERRATUM_826319) || \
	defined(CONFIG_ARM64_ERRATUM_827319) || \
	defined(CONFIG_ARM64_ERRATUM_824069)
	{
	/* Cortex-A53 r0p[012] */
		.desc = "ARM errata 826319, 827319, 824069",
		.capability = ARM64_WORKAROUND_CLEAN_CACHE,
		MIDR_RANGE(MIDR_CORTEX_A53, 0x00, 0x02),
		.enable = cpu_enable_cache_maint_trap,
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_819472
	{
	/* Cortex-A53 r0p[01] */
		.desc = "ARM errata 819472",
		.capability = ARM64_WORKAROUND_CLEAN_CACHE,
		MIDR_RANGE(MIDR_CORTEX_A53, 0x00, 0x01),
		.enable = cpu_enable_cache_maint_trap,
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_832075
	{
	/* Cortex-A57 r0p0 - r1p2 */
		.desc = "ARM erratum 832075",
		.capability = ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE,
		MIDR_RANGE(MIDR_CORTEX_A57, 0x00,
			   (1 << MIDR_VARIANT_SHIFT) | 2),
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_834220
	{
	/* Cortex-A57 r0p0 - r1p2 */
		.desc = "ARM erratum 834220",
		.capability = ARM64_WORKAROUND_834220,
		MIDR_RANGE(MIDR_CORTEX_A57, 0x00,
			   (1 << MIDR_VARIANT_SHIFT) | 2),
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_845719
	{
	/* Cortex-A53 r0p[01234] */
		.desc = "ARM erratum 845719",
		.capability = ARM64_WORKAROUND_845719,
		MIDR_RANGE(MIDR_CORTEX_A53, 0x00, 0x04),
	},
#endif
#ifdef CONFIG_CAVIUM_ERRATUM_23154
	{
	/* Cavium ThunderX, pass 1.x */
		.desc = "Cavium erratum 23154",
		.capability = ARM64_WORKAROUND_CAVIUM_23154,
		MIDR_RANGE(MIDR_THUNDERX, 0x00, 0x01),
	},
#endif
#ifdef CONFIG_CAVIUM_ERRATUM_27456
	{
	/* Cavium ThunderX, T88 pass 1.x - 2.1 */
		.desc = "Cavium erratum 27456",
		.capability = ARM64_WORKAROUND_CAVIUM_27456,
		MIDR_RANGE(MIDR_THUNDERX, 0x00,
			   (1 << MIDR_VARIANT_SHIFT) | 1),
	},
	{
	/* Cavium ThunderX, T81 pass 1.0 */
		.desc = "Cavium erratum 27456",
		.capability = ARM64_WORKAROUND_CAVIUM_27456,
		MIDR_RANGE(MIDR_THUNDERX_81XX, 0x00, 0x00),
	},
#endif
	{
		.desc = "Mismatched cache line size",
		.capability = ARM64_MISMATCHED_CACHE_LINE_SIZE,
		.matches = has_mismatched_cache_line_size,
		.def_scope = SCOPE_LOCAL_CPU,
		.enable = cpu_enable_trap_ctr_access,
	},
#ifdef CONFIG_HARDEN_BRANCH_PREDICTOR
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CORTEX_A57),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CORTEX_A72),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CORTEX_A73),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CORTEX_A75),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_BRCM_VULCAN),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CAVIUM_THUNDERX2),
		.enable = enable_smccc_arch_workaround_1,
	},
	{
		.capability = ARM64_HARDEN_BRANCH_PREDICTOR,
		MIDR_ALL_VERSIONS(MIDR_CORTEX_ENYO),
		.enable = enable_smccc_arch_workaround_1,
	},
#endif
#ifdef CONFIG_ARM64_SSBD
	{
		.desc = "Speculative Store Bypass Disable",
		.capability = ARM64_SSBD,
		.def_scope = SCOPE_LOCAL_CPU,
		.matches = has_ssbd_mitigation,
	},
#endif
	{
	}
};

/*
 * The CPU Errata work arounds are detected and applied at boot time
 * and the related information is freed soon after. If the new CPU requires
 * an errata not detected at boot, fail this CPU.
 */
void verify_local_cpu_errata_workarounds(void)
{
	const struct arm64_cpu_capabilities *caps = arm64_errata;

	for (; caps->matches; caps++) {
		if (cpus_have_cap(caps->capability)) {
			if (caps->enable)
				caps->enable((void *)caps);
		} else if (caps->matches(caps, SCOPE_LOCAL_CPU)) {
			pr_crit("CPU%d: Requires work around for %s, not detected"
					" at boot time\n",
				smp_processor_id(),
				caps->desc ? : "an erratum");
			cpu_die_early();
		}
	}
}

void update_cpu_errata_workarounds(void)
{
	update_cpu_capabilities(arm64_errata, "enabling workaround for");
}

void __init enable_errata_workarounds(void)
{
	enable_cpu_capabilities(arm64_errata);
}
