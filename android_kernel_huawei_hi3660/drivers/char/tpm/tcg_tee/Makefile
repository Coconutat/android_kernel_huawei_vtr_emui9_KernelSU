#
# Makefile for the kernel tpm on TEE  drivers.
#
obj-$(CONFIG_TCG_TEE) += tpm_tee.o

ifdef CONFIG_TCG_TEE
ccflags-y += -Idrivers/hisi/tzdriver -Idrivers/char/tpm/
endif
