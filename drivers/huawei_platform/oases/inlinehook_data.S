.section ".data","aw"

.global oases_handler_func_pre_start
.global oases_handler_func_pre_end

.balign 4
oases_handler_func_pre_start:
.incbin "inlinehook_func_pre.bin"
.balign 4
oases_handler_func_pre_end:

.global oases_handler_func_post_start
.global oases_handler_func_post_end

.balign 4
oases_handler_func_post_start:
.incbin "inlinehook_func_post.bin"
.balign 4
oases_handler_func_post_end:

.global oases_handler_func_pre_post_start
.global oases_handler_func_pre_post_end

.balign 4
oases_handler_func_pre_post_start:
.incbin "inlinehook_func_pre_post.bin"
.balign 4
oases_handler_func_pre_post_end:

.global oases_handler_subfunc_pre_start
.global oases_handler_subfunc_pre_end

.balign 4
oases_handler_subfunc_pre_start:
.incbin "inlinehook_subfunc_pre.bin"
.balign 4
oases_handler_subfunc_pre_end:

.global oases_handler_subfunc_post_start
.global oases_handler_subfunc_post_end

.balign 4
oases_handler_subfunc_post_start:
.incbin "inlinehook_subfunc_post.bin"
.balign 4
oases_handler_subfunc_post_end:
