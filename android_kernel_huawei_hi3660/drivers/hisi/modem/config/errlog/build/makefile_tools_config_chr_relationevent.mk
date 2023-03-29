#***********************************************************#
#***********************************************************#
include $(BALONG_TOPDIR)/build/scripts/make_base.mk

#***********************************************************#
# moudle name
#***********************************************************#
OBC_LOCAL_MOUDLE_NAME   ?=tools_config_chr

#***********************************************************#
# compiler flags
#***********************************************************#
CC_USER_FLAGS   ?=
AS_USER_FLAGS   ?=

ifeq ($(CFG_CCPU_OS),RTOSCK_SMP)
CC_USER_FLAGS += -march=armv7-r
else
CC_USER_FLAGS += -march=armv7-a
endif

CC_USER_FLAGS += -gdwarf-2

#***********************************************************#
# compiler defines
#***********************************************************#
CC_USER_DEFINES ?=
AS_USER_DEFINES ?=

#***********************************************************#
# include Directories
#***********************************************************#
#注：在这里添加编译的头文件

OBC_LOCAL_INC_DIR ?=
OBC_LOCAL_INC_DIR += \
    $(BALONG_TOPDIR)/ \
    $(BALONG_TOPDIR)/modem/platform/dsp/${CFG_PLATFORM_HISI_BALONG}/ \
    $(BALONG_TOPDIR)/modem/config/errlog/chrdirectrpt/inc \
    $(BALONG_TOPDIR)/modem/config/product/$(OBB_PRODUCT_NAME)/config    \
    $(BALONG_TOPDIR)/modem/ps/comm/comm/ccore/MEM/NODE/Inc 

#***********************************************************#
# source files
#***********************************************************#
#注：在这里添加需要编译的源文件
OBC_LOCAL_SRC_FILE := $(BALONG_TOPDIR)/modem/config/errlog/chrdirectrpt/converter/deploy/bin/chr_struconvert_relationevent.c

include $(BALONG_TOPDIR)/modem/ps/build/tl/TOOLS_CONFIG/makefile_tools_config_chr_tlas.mk
include $(BALONG_TOPDIR)/modem/ps/build/tl/TOOLS_CONFIG/makefile_tools_config_chr_tlnas.mk
include $(BALONG_TOPDIR)/modem/ps/build/gu/TOOLS_CONFIG/makefile_tools_config_chr_gas.mk
include $(BALONG_TOPDIR)/modem/ps/build/gu/TOOLS_CONFIG/makefile_tools_config_chr_gucnas.mk
include $(BALONG_TOPDIR)/modem/ps/build/cdma/tools_config/makefile_tools_config_chr_cttf.mk
include $(BALONG_TOPDIR)/modem/ps/build/cdma/tools_config/makefile_tools_config_chr_cas.mk
include $(BALONG_TOPDIR)/modem/ps/build/gu/TOOLS_CONFIG/makefile_tools_config_chr_was.mk
#***********************************************************
#include rules. must be droped at the bottom, OBB_BUILD_ACTION values: cc tqe lint fortify
#***********************************************************
#注：在此添加需要引用的编译脚本
include $(BALONG_TOPDIR)/build/scripts/rules/$(OBB_BUILD_ACTION)_rtosck_rules.mk
