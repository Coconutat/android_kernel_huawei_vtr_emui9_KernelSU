#
# Makefile for the memory technology device drivers.
#

# Core functionality.

obj-y	+= avs_balong.o

subdir-ccflags-y := -Werror

ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE_avs_balong.o := y
endif