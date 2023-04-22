#include "rasprobe.h"

#ifdef __ASM_PTRACE_H
/*ARM64bit arm arg's regs : r0-r3 */
#define REG_RET(regs)  regs->regs[0]
#define REG_ARG0(regs) regs->regs[0]
#define REG_ARG1(regs) regs->regs[1]
#define REG_ARG2(regs) regs->regs[2]
#define REG_ARG3(regs) regs->regs[3]

#elif defined __ASM_ARM_PTRACE_H
/*ARM32bit arm arg's regs : r0-r3*/
#define REG_RET(regs)  regs->uregs[0]
#define REG_ARG0(regs) regs->uregs[0]
#define REG_ARG1(regs) regs->uregs[1]
#define REG_ARG2(regs) regs->uregs[2]
#define REG_ARG3(regs) regs->uregs[3]

#else
/*x86 Linux 9 arg's regs : RDI,RSI,RDX,RCX,RAX,R8,R9,R10,R11*/
#define REG_RET(regs) regs->ax
#define REG_ARG0(regs) regs->di
#define REG_ARG1(regs) regs->si
#define REG_ARG2(regs) regs->dx
#define REG_ARG3(regs) regs->cx
#endif

int rasprobe_entry(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct RasRegs *rd = (struct RasRegs *)ri->data;
	/*arm arg's regs : r0-r3*/
	rd->args[0] = (void *)REG_ARG0(regs);
	rd->args[1] = (void *)REG_ARG1(regs);
	rd->args[2] = (void *)REG_ARG2(regs);
	rd->args[3] = (void *)REG_ARG3(regs);
	return 0;
}

void rasprobe_seturn(struct pt_regs *regs, long val)
{
	REG_RET(regs) = val;
}

void rasprobe_setarg(struct pt_regs *regs, int index, long val)
{
	switch (index) {
	case 0:
		REG_ARG0(regs) = val;
		break;
	case 1:
		REG_ARG1(regs) = val;
		break;
	case 2:
		REG_ARG2(regs) = val;
		break;
	case 3:
		REG_ARG3(regs) = val;
		break;
	default:
		break;
	}
}
int register_rasprobes(struct rasprobe **rps, int num)
{
	int i, ret;

	if (num <= 0)
		return -EINVAL;
	for (i = 0; i < num; i++) {
		ret = register_kretprobe(rps[i]);
		if (ret < 0) {
			unregister_rasprobes(rps, i);
			break;
		}
	}
	return ret;
}

void unregister_rasprobes(struct rasprobe **rps, int num)
{
	int i;

	for (i = 0; i < num; i++)
		unregister_kretprobe(rps[i]);
}
