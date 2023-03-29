# Created by Jeffery.zhai/199916 2012.03.21

#***********************************************************#
# include the define at the top
#***********************************************************#
include $(BALONG_TOPDIR)/build/scripts/make_base.mk

MOUDULES := tools_config_chr

# process
.PHONY: tools_config_chr
tools_config_chr:
	$(MAKE)  -f makefile_tools_config_chr_relationevent.mk

#clean
CLEAN_MOUDULES:= tools_config_chr

# process
.PHONY: clean-tools_config_chr
clean-tools_config_chr:
	$(MAKE)  -f makefile_tools_config_chr_relationevent.mk clean
