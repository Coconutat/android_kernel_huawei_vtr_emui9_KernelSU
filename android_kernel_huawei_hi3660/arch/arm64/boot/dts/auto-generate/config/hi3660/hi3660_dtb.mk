#Copyright Huawei Technologies Co., Ltd. 1998-2011. All rights reserved.
#This file is Auto Generated 

dtb-y += hi3660/hi3660_udp_EVB8_VERC_config.dtb
dtb-y += hi3660/hi3660_udp_EVB8_VERD_v120_config.dtb
dtb-y += hi3660/hi3660_udp_EVB8_VERD_config.dtb
dtb-y += hi3660/hi3660_udp_v120_config.dtb
dtb-y += hi3660/hi3660_udp_EVB8_VERD_v110_config.dtb
dtb-y += hi3660/hi3660_udp_default_config.dtb
dtb-y += hi3660/hi3660_udp_v110_config.dtb

targets += hi3660_dtb
targets += $(dtb-y)

# *.dtb used to be generated in the directory above. Clean out the
# old build results so people don't accidentally use them.
hi3660_dtb: $(addprefix $(obj)/, $(dtb-y))
	$(Q)rm -f $(obj)/../*.dtb

clean-files := *.dtb

#end of file
