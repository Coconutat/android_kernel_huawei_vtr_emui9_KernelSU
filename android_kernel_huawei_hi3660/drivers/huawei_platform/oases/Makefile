#
#	OASES Project to patch android kernel
#
#	Copyright (C) 2016 Baidu, Inc.
#

OASES_SRC := $(src)
OASES_OBJ := $(obj)
OASES_SRCTREE := $(srctree)/$(src)
OASES_OBJTREE := $(objtree)/$(obj)

# set to 0 if not need, default 1.
ccflags-y += -DOASES_ENABLE_REPLACEMENT_HANDLER=1

# set to 1 if kernel CFI enabled, default 0
ccflags-y += -DOASES_ENABLE_CFI=1

# set to 1 to enable precise patch(OASES_ENABLE_REPLACEMENT_HANDLER=1 required), default 0.
ccflags-y += -DOASES_ENABLE_PRECISE_PATCH=0

# comment out when release to reduce size
ccflags-y += -DDEBUG

# for inlinehook_offset.h
ccflags-y += -I$(OASES_OBJ)

# for OASES_INLINEHOOK_AUTOGEN
asflags-y += -I$(OASES_OBJ)

OASES_INLINEHOOK_AUTOGEN := \
	inlinehook_func_pre.bin \
	inlinehook_func_post.bin \
	inlinehook_func_pre_post.bin \
	inlinehook_subfunc_pre.bin \
	inlinehook_subfunc_post.bin

OASES_INLINEHOOK_AUTOGEN := $(addprefix $(OASES_OBJ)/, $(OASES_INLINEHOOK_AUTOGEN))

$(OASES_OBJ)/inlinehook_data.o: $(OASES_INLINEHOOK_AUTOGEN)

# compatible with old toolchains with weird behavior on as/ld flags
$(OASES_OBJ)/inlinehook_data.o: $(OASES_SRCTREE)/inlinehook_data.S
	$(CC) -c -o $@ -Wa,-I$(OASES_OBJ) $<

$(OASES_OBJ)/inlinehook_%.bin: $(OASES_OBJ)/inlinehook_%.oas
	$(OBJCOPY) -O binary $< $@

$(OASES_OBJ)/inlinehook_%.oas: $(OASES_SRCTREE)/inlinehook_%.S FORCE
	$(CC) -c -o $@ $<

$(OASES_OBJ)/inlinehook_offset.h: $(OASES_INLINEHOOK_AUTOGEN)
	$(OASES_SRCTREE)/inlinehook_gen_off $@ $(OASES_SRCTREE)/inlinehook_const.h $^

$(OASES_OBJ)/hook_func.o: $(OASES_OBJ)/inlinehook_offset.h
$(OASES_OBJ)/hook_subfunc.o: $(OASES_OBJ)/inlinehook_offset.h

obj-$(CONFIG_OASES) += \
	util.o \
	vmarea.o \
	plts.o \
	kallsyms.o \
	inlinehook.o \
	inlinehook_data.o \
	patch_addr.o \
	patch_base.o \
	hook_insn.o \
	hook_func.o \
	hook_subfunc.o \
	sysfs.o \
	patch_mgr.o \
	patch_api.o \
	patch_file.o \
	oases.o

obj-$(CONFIG_OASES_SIGNATURE_SUPPORT) += \
	oases_sign_certificate.o \
	vendor_sign_certificate.o \
	oases_signing.o

obj-$(CONFIG_OASES_SIGNATURE_SUPPORT) += sign/

obj-$(CONFIG_OASES_STATIC_PLTS) += plts_static.o

$(OASES_OBJ)/oases_sign_certificate.o: $(OASES_SRCTREE)/oases_sign_certificate.S $(OASES_SRCTREE)/oases_key.x509 FORCE
	$(CC) -c -o $@ -Wa,-I$(OASES_SRCTREE) $<

$(OASES_OBJ)/vendor_sign_certificate.o: $(OASES_SRCTREE)/vendor_sign_certificate.S $(OASES_SRCTREE)/vendor_key.x509 FORCE
	$(CC) -c -o $@ -Wa,-I$(OASES_SRCTREE) $<

OASES_INLINEHOOK_AUTOGEN := $(addprefix $(objtree)/, $(OASES_INLINEHOOK_AUTOGEN))

clean-files += $(OASES_INLINEHOOK_AUTOGEN) \
	$(OASES_OBJTREE)/inlinehook_offset.h
