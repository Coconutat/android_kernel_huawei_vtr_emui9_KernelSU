#include "inlinehook_const.h"

#if defined(__aarch64__)

func_post:
	sub sp, sp, #0xa0
	stp x29, x30, [sp]
	mov x29, sp
	stp x0, x1, [sp, #0x18]
	stp x2, x3, [sp, #0x28]
	stp x4, x5, [sp, #0x38]
	stp x6, x7, [sp, #0x48]
	adr x30, orig_lr
place_holder:
.word PLACE_HOLDER_MAGIC
	mov x0, x0
	mov x0, x0
	mov x0, x0
	ldr x16, orig_addr
	blr x16
orig_lr:
	stp x0, x1, [sp, #0x58]
	stp x2, x3, [sp, #0x68]
	stp x4, x5, [sp, #0x78]
	stp x6, x7, [sp, #0x88]
    str x0, [x29, #0x10]
    ldp x6, x7, [sp, #0x40]
    ldp x4, x5, [sp, #0x30]
    ldp x2, x3, [sp, #0x20]
    ldp x0, x1, [sp, #0x10]
    add x0, x29, #0x10
    ldr x16, filter_addr
    blr x16
    cbz x0, not_attack_log
    ldr x0, patch_info
    ldr x1, attack_logger
    blr x1
not_attack_log:
	ldp x6, x7, [sp, #0x88]
	ldp x4, x5, [sp, #0x78]
	ldp x2, x3, [sp, #0x68]
	ldp x0, x1, [sp, #0x58]
    ldr x0, [x29, #0x10]
    ldp x29, x30, [sp]
    add sp, sp, #0xa0
    ret

filter_addr:
.quad FILTER_ADDR_MAGIC
orig_addr:
.quad ORIGIN_ADDR_MAGIC
patch_info:
.quad PATCH_INFO_CTX_MAGIC
attack_logger:
.quad ATTACK_LOGGER_MAGIC

#elif defined(__arm__)

.arm

func_post:
	@ assume at most 4 args on stack
	@ assume all argument size are within 32 bits
	@ stack layout:
	@ ret, padding * 2, cpsr, r0-r11, lr, args * 4, cpsr, r0-r11, lr, padding, args * 4, = 0xa0
	sub sp, #0xa0                       @ save state
	mrs r12, cpsr
	str r12, [sp, #0x90]
    add r12, sp, #0x90
	stmfd r12, {r0-r11,lr}
	ldr r12, [sp, #0xa0]                @ copy possible args
	str r12, [sp, #0x4c]
	str r12, [sp, #0]
	ldr r12, [sp, #0xa4]
	str r12, [sp, #0x50]
	str r12, [sp, #4]
	ldr r12, [sp, #0xa8]
	str r12, [sp, #0x54]
	str r12, [sp, #8]
	ldr r12, [sp, #0xac]
	str r12, [sp, #0x58]
	str r12, [sp, #0xc]
	add r12, sp, #0x5c                  @ restore state
	ldmfd r12, {r0-r11}
	adr lr, orig_lr                     @ call original function
	ldr r12, [sp, #0x90]
	msr cpsr_c, r12
place_holder:
	.word PLACE_HOLDER_MAGIC
	mov r0, r0
	mov r0, r0
	mov r0, r0
	ldr pc, orig_addr
orig_lr:
	add r12, sp, #0x48
	stmfd r12, {r0-r11,lr}              @ save state after calling
	mrs r12, cpsr
	str r12, [sp, #0x48]
	str r0, [sp, #0x9c]                 @ save return value
	ldr r12, [sp, #0x68]                @ move old r3 to stack
	str r12, [sp, #0]
	ldr r12, [sp, #0x4c]                @ copy old args to stack
	str r12, [sp, #4]
	ldr r12, [sp, #0x50]
	str r12, [sp, #8]
	ldr r12, [sp, #0x54]
	str r12, [sp, #0xc]
	ldr r12, [sp, #0x58]
	str r12, [sp, #0x10]
	add r0, sp, #0x9c
	ldr r1, [sp, #0x5c]
	ldr r2, [sp, #0x60]
	ldr r3, [sp, #0x64]
	ldr r12, filter_addr                @ call filter
	blx r12
	cmp r0, #0
	beq not_attack_log
	ldr r0, patch_info
	ldr r1, attack_logger
	blx r1
not_attack_log:
    add r12, sp, #0x14
	ldmfd r12, {r0-r11}
	ldr lr, [sp, #0x8c]
	ldr r0, [sp, #0x9c]
	ldr r12, [sp, #0x48]
    add sp, #0xa0
	msr cpsr_c, r12
	bx lr

filter_addr:
.word FILTER_ADDR_MAGIC
orig_addr:
.word ORIGIN_ADDR_MAGIC
patch_info:
.word PATCH_INFO_CTX_MAGIC
attack_logger:
.word ATTACK_LOGGER_MAGIC

#endif
