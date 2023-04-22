/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef _MALI_KBASE_CONFIG_PLATFORM_H_
#define _MALI_KBASE_CONFIG_PLATFORM_H_

/**
 * Maximum frequency GPU will be clocked at. Given in kHz.
 * This must be specified as there is no default value.
 *
 * Attached value: number in kHz
 * Default value: NA
 */
#define GPU_FREQ_KHZ_MAX 5000
/**
 * Minimum frequency GPU will be clocked at. Given in kHz.
 * This must be specified as there is no default value.
 *
 * Attached value: number in kHz
 * Default value: NA
 */
#define GPU_FREQ_KHZ_MIN 5000

#define POWER_MANAGEMENT_CALLBACKS		(&pm_callbacks)

#define KBASE_PLATFORM_CALLBACKS                ((uintptr_t)&platform_funcs)

#ifdef CONFIG_PM_DEVFREQ
#define POWER_MODEL_CALLBACKS		((uintptr_t)&hisi_model_ops)
#endif

#define GPU_SPEED_FUNC (NULL)

#define CPU_SPEED_FUNC (&kbase_cpuprops_get_default_clock_speed)

#define PLATFORM_FUNCS (KBASE_PLATFORM_CALLBACKS)

/**
 * @brief Tell whether a feature should be enabled
 */
#define kbase_has_hi_feature(kbdev, hi_feature)\
	test_bit(hi_feature, &(kbdev)->hi_features_mask[0])

/*
 * Begin Register Offsets
 */


#define KBASE_PWR_KEY_VALUE             0x2968a819
#define KBASE_PWR_OVERRIDE_VALUE        0xc4b00960

#if defined (CONFIG_MALI_TRYM)
#define SYS_REG_PMCTRL_BASE_ADDR        0xFFF01000
#define SYS_REG_CRG_BASE_ADDR           0xFFF05000 /* Crg control register base address */
#define SYS_REG_PCTRL_BASE_ADDR         0xFE02E000
#define PERI_CTRL19                     0x0E0
#else
#define SYS_REG_PMCTRL_BASE_ADDR        0xFFF31000
#define SYS_REG_CRG_BASE_ADDR           0xFFF35000 /* Crg control register base address */
#define SYS_REG_PCTRL_BASE_ADDR         0xE8A09000
#define PERI_CTRL19                     0x050
#endif

#define SYS_REG_PMCTRL_SIZE             0x1000
#define SYS_REG_CRG_SIZE                0x1000 /* Crg control register size */

#define G3DHPMBYPASS                    0x264
#define PERI_CTRL93                     0x234
#define MASK_PERI_CTRL93                0x70000

#if defined (CONFIG_MALI_GONDUL)
#define PERI_STAT_FPGA_GPU_EXIST        0xC4
#define PERI_STAT_FPGA_GPU_EXIST_MASK   0x4
#elif defined (CONFIG_MALI_TRYM)
#define PERI_STAT_FPGA_GPU_EXIST        0xBC
#define PERI_STAT_FPGA_GPU_EXIST_MASK   0x40
#else
#define PERI_STAT_FPGA_GPU_EXIST        0xBC
#define PERI_STAT_FPGA_GPU_EXIST_MASK   0x400000
#endif

#if defined (CONFIG_MALI_MIMIR) || defined (CONFIG_MALI_SIGURD)
#define G3DAUTOCLKDIVBYPASS             0x268
#define VS_CTRL_2                       0x46c
#elif defined (CONFIG_MALI_HEIMDALL)
#define G3DAUTOCLKDIVBYPASS             0x268
#define VS_CTRL_2                       0x418
#elif defined (CONFIG_MALI_NORR)
#define G3DAUTOCLKDIVBYPASS             0x258
#define VS_CTRL_2                       0x448
#elif defined (CONFIG_MALI_TRYM) || defined (CONFIG_MALI_GONDUL)
#define G3DAUTOCLKDIVBYPASS             0x1D8
#define VS_CTRL_2                       0x448
#endif
/* for cs */
#define G3DAUTOCLKDIVBYPASS_2           0x248
#define VS_CTRL_GPU                     0x448

#define MASK_AUTOSDBYHW                 0x80000000
#define MASK_ENABLEDSBYSF               0x3FFFE
#define MASK_DISABLEDSBYSF              0xFFFC0001

#define PERI_CTRL21                     0x58
#define MASK_ENABLESDBYSF               0x1FFFF
#define MASK_DISABLESDBYSF              0xFFFE0000

#define MASK_G3DHPMBYPASS               0xfff0fff0
#define MASK_G3DAUTOCLKDIVBYPASS        0xfffffffe

#define SYS_REG_PCTRL_SIZE              0x1000
#define GPU_X2P_GATOR_BYPASS            0xfeffffff



#define SYS_REG_CRG_CLOCK_EN            0x38
#define SYS_REG_CRG_CLCOK_STATUS        0x3c
#define SYS_REG_CRG_G3D                 0x84
#define SYS_REG_CRG_G3D_EN              0x88
#define SYS_REG_CRG_RESET_STATUS        0x8c
#define SYS_REG_CRG_ISO_STATUS          0x14c

#define KBASE_PWR_RESET_VALUE           0x007c001c
#define KBASE_PWR_ACTIVE_BIT            0x2
#define KBASE_PWR_INACTIVE_MAX_LOOPS    100000

#define SYS_REG_CRG_W_CLOCK_EN          0x30
#define SYS_REG_CRG_W_CLOCK_CLOSE       0x34
#define SYS_REG_CRG_CLK_DIV_MASK_EN     0xf0

#define GPU_CRG_CLOCK_VALUE             0x00000038
#define GPU_CRG_CLOCK_POWER_OFF_MASK    0x00010000
#define GPU_CRG_CLOCK_POWER_ON_MASK     0x00010001

extern struct kbase_pm_callback_conf pm_callbacks;

extern struct kbase_platform_funcs_conf platform_funcs;
extern void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work);

#endif /* _MALI_KBASE_CONFIG_PLATFORM_H_ */
