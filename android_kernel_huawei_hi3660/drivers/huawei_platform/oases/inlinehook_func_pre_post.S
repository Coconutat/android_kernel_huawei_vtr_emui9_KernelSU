#include "inlinehook_const.h"

#if defined(__aarch64__)

func_pre_post:
.word PLACE_HOLDER_MAGIC
	mov x0, x0
	mov x0, x0
	mov x0, x0
	ldr x16, orig_addr
	br x16

orig_addr:
.quad ORIGIN_ADDR_MAGIC

#elif defined(__arm__)

.arm

func_pre_post:
.word PLACE_HOLDER_MAGIC
	mov r0, r0
	mov r0, r0
	mov r0, r0
	ldr pc, orig_addr

orig_addr:
.word ORIGIN_ADDR_MAGIC

#endif