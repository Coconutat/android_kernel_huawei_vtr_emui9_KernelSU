#
#kernel dump Makefile
#
subdir-ccflags-y              += -Wall -Werror -DTEXT_OFFSET=$(TEXT_OFFSET)
obj-$(CONFIG_HISI_KERNELDUMP) += kernel_dump.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver/libhwsecurec