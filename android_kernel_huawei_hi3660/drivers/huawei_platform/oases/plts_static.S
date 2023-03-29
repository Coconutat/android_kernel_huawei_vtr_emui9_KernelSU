#include <linux/init.h>
#include <linux/const.h>
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <asm/page.h>
#include "plts_const.h"

.balign PAGE_SIZE

.global oases_plts
oases_plts:
    .zero OASES_PLTS_MAX_SIZE
