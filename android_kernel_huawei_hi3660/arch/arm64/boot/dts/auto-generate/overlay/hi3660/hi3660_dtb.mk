#Copyright Huawei Technologies Co., Ltd. 1998-2011. All rights reserved.
#This file is Auto Generated 


targets += hi3660_dtbo
targets += $(dtb-y)

# *.dtbo used to be generated in the directory above. Clean out the
# old build results so people don't accidentally use them.
hi3660_dtbo: $(addprefix $(obj)/, $(dtb-y))
	$(Q)rm -f $(obj)/../*.dtbo

clean-files := *.dtbo

#end of file
