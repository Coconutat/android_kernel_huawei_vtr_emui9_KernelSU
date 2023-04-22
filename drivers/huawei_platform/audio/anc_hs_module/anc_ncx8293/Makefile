ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DANC_NCX8293_DEBUG
endif
obj-$(CONFIG_ANC_NCX8293)	:= anc_ncx8293.o
ccflags-$(CONFIG_ANC_NCX8293) += -Idrivers/devkit/audiokit/
