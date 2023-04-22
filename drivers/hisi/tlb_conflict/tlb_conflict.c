/*
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
#include <linux/signal.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/kexec.h>
#include <linux/init.h>
#include <linux/sched.h>

#include <asm/esr.h>
#include <asm/traps.h>
#include <asm/exception.h>
#include <asm/system_misc.h>

/*lint -e438 -e529 -e550 -e715*/
extern asmlinkage void bad_mode(struct pt_regs *regs, int reason, unsigned int esr);

#define ESR_IABT_CURR_EL_IFSC_MASK    0x3f
#define ESR_IABT_CURR_EL_TLB_CONFLICT 0x30
asmlinkage void __exception do_iabt_curr_el(unsigned long addr,
	unsigned int esr, struct pt_regs *regs)
{
	unsigned int ifsc = esr & ESR_IABT_CURR_EL_IFSC_MASK;
	unsigned long page_no = 0;

	if (ifsc == ESR_IABT_CURR_EL_TLB_CONFLICT) {
		page_no = addr >> PAGE_SHIFT;

		dsb(ishst);
		asm("tlbi vaae1is, %0" ::"r"(page_no));
		dsb(ish);
		isb();

		return;
	}

	bad_mode(regs, 0, esr);

	return;
}

int do_tlb_conflict(unsigned long addr, unsigned int esr,
	struct pt_regs *regs)
{
	unsigned long page_no = addr >> PAGE_SHIFT;

	dsb(ishst);

	asm("tlbi vaae1is, %0" ::"r"(page_no));

	dsb(ish);
	isb();

	return 0;
}
/*lint +e438 +e529 +e550 +e715*/

