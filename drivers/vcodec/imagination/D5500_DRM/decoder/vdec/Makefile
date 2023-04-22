###############################################################################
#
# Copyright (c) Imagination Technologies Ltd.
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a 
# copy of this software and associated documentation files (the "Software"), 
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
# THE SOFTWARE.
# 
# Alternatively, the contents of this file may be used under the terms of the 
# GNU General Public License Version 2 ("GPL")in which case the provisions of
# GPL are applicable instead of those above. 
# 
# If you wish to allow use of your version of this file only under the terms 
# of GPL, and not to allow others to use your version of this file under the 
# terms of the MIT license, indicate your decision by deleting the provisions 
# above and replace them with the notice and other provisions required by GPL 
# as set out in the file called “GPLHEADER” included in this distribution. If 
# you do not delete the provisions above, a recipient may use your version of 
# this file under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called 
# "MIT_COPYING".
#
###############################################################################

#
# kernel makefile template for module vdecdd
#

KBUILD_CFLAGS := $(filter-out -Werror,$(KBUILD_CFLAGS))

obj-y := vdecdd.o

ccflags-y += -g -DVDEC_MSVDX_HARDWARE -DVDEC -DUSE_SHARING -DUSE_FW_CTX_TRIMMING -DUSE_FW_RELOC_INFO_PACKING -DH264_ENABLE_MVC -DH264_SECOND_PASS_DEBLOCK -DENABLE_SEPARATE_COLOUR_PLANES -D__RELEASE__="release" -D__RELEASE_RELEASE__ -DNDEBUG -DVDEC_TARGET_ -D_FILE_OFFSET_BITS=64 -D__linux__ -DNO_REGIO_CHECK_FIELD_VALUE -DFAKE_DEVIF -DENABLE_NULL_DEVIF -DSECURE_USE_SYSOS -DUSE_REAL_FW -DOMX_SHARED -DVDEC_USE_PVDEC -DVDEC_USE_PVDEC_COMPATIBILITY -DERROR_CONCEALMENT -DVDEC_USE_PVDEC_SEC -DPVDEC_SEC_TEE_PHYADDR -D__PORT_FWRK__ -DSYSBRG_BRIDGING -DSEC_USE_REAL_FW -DUSE_MTX_DMA_LOAD -D__TALMMU_USE_SYSAPIS__ -D__TALMMU_NO_TAL__ -DSECURE_TAL -DUSE_STRICT_MIN_PIC_SIZE_CHECK -DSEC_USE_FW_RELOC_INFO_PACKING -DSEC_USE_FW_CTX_TRIMMING -DTARGET_HEADER_NAME=\"vxd_pvdec.h\" -DIMG_KERNEL_MODULE
ccflags-y +=  -I$(src)/../../decoder/vdec/regdefs/pvdec/include -I$(src)/../../decoder/vdec/kernel_device/libraries/null/../../include -I$(src)/../../imgvideo/include -I$(src)/../../imgvideo/include/linux-kernel -I$(src)/../../imgvideo/system/include -I$(src)/../../imgvideo/port_fwrk/include -I$(src)/../../imgvideo/list_utils/include -I$(src)/../../imgvideo/secure_media/target/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/../../include -I$(src)/../../decoder/vdec/tee/secure_api -I$(src)/../../decoder/vdec/system/vdec -I$(src)/../../decoder/vdec/apis/vdec/include -I$(src)/../../decoder/vdec/platform/include -I$(src)/../../decoder/vdec/platform -I$(src)/../../decoder/vdec/apis/vdec/libraries/include -I$(src)/../../decoder/vdec/firmware/include -I$(src)/../../decoder/vdec/firmware/share -I$(src)/../../decoder/vdec/firmware/share/c99 -I$(src)/../../imgvideo/imglib/libraries/pixelapi/include -I$(src)/../../imgvideo/imglib/libraries/RegDefsUtils/include -I$(src)/../../imgvideo/imglib/libraries/listman/include -I$(src)/../../imgvideo/imglib/libraries/cbman/include -I$(src)/../../decoder/vdec/secure_media/secure_img/libraries/bspp/include -I$(src)/../../imgvideo/kernel_comp/include -I$(src)/../../decoder/vdec/secure_media/secure_mem/include -I$(src)/../../decoder/vdec/secure_media/secure_mem/../include -I$(src)/../../decoder/vdec/secure_media/secure_img/include -I$(src)/../../imgvideo/rpc/sysbrg/src -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/libcomp/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/libcomp/pvdec/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/libcomp/pvdec/../decoder/include -I$(src)/../../decoder/vdec/firmware_pvdec -I$(src)/../../imgvideo/imglib/libraries/talmmu_api/include -I$(src)/../../imgvideo/secure_media/tal/include -I$(src)/../../decoder/vdec/secure_media/secure_img/libraries/device_interface/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/libcomp/decoder/include -I$(src)/../../decoder/vdec/kernel_device/libraries/vdecdd/libcomp/decoder/../pvdec/include -I$(src)/../../decoder/vdec/secure_media/secure_mem/libraries/mem/include -I$(src)/../../imgvideo/platform/include/uapi -I$(src)/../../imgvideo/platform/include -I$(src)/../../decoder/vdec/firmware/bin -I$(src)/../../../../../tzdriver/ 

ifeq ($(wildcard drivers/staging/android/ion/ion.h),)
ccflags-y += -DIMG_KERNEL_ION_HEADER="<linux/ion.h>"
else
ccflags-y += -DIMG_KERNEL_ION_HEADER="<$(srctree)/drivers/staging/android/ion/ion.h>"
endif

ifneq ( ,$(findstring arm64,$(ARCH)))
ccflags-y += -fno-pic
endif

vdecdd-objs :=  kernel_device/libraries/null/code/null_device_km.o kernel_device/libraries/vdecdd/libcomp/pvdec/code/devaio.o kernel_device/libraries/vdecdd/libcomp/pvdec/code/pvdec_int.o kernel_device/libraries/vdecdd/libcomp/pvdec/code/pvdec_msg.o kernel_device/libraries/vdecdd/libcomp/pvdec/code/pvdec_io.o kernel_device/libraries/vdecdd/libcomp/pvdec/code/pvdecsec_int.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/h264_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/h264_idx.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/mpeg4_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/mpeg4_idx.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vp8_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vp8_idx.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vp6_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vp6_idx.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/mpeg2_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/mpeg2_idx.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vc1_vlc.o kernel_device/libraries/vdecdd/libcomp/vlc_tables/code/vc1_idx.o kernel_device/libraries/vdecdd/libcomp/decoder/code/decoder.o kernel_device/libraries/vdecdd/libcomp/decoder/code/hwctrl_api.o kernel_device/libraries/vdecdd/libcomp/decoder/code/translation_api.o kernel_device/libraries/vdecdd/libcomp/decoder/code/scaler_setup.o kernel_device/libraries/vdecdd/libcomp/decoder/code/dec_resources.o kernel_device/libraries/vdecdd/libcomp/decoder/code/vxd_int.o kernel_device/libraries/vdecdd/libcomp/decoder/code/fwbsp_bridge.o kernel_device/libraries/vdecdd/code/core_api.o kernel_device/libraries/vdecdd/code/plant.o kernel_device/libraries/vdecdd/code/resource.o kernel_device/libraries/vdecdd/code/scheduler.o kernel_device/libraries/vdecdd/code/vdecdd_mmu.o kernel_device/libraries/vdecdd/code/vdecdd_km.o kernel_device/libraries/vdecdd/libcomp/vdecdd_utils/code/vdecdd_utils.o kernel_device/libraries/vdecdd/libcomp/vdecdd_utils/code/vdecdd_utils_buf.o secure_media/secure_img/libraries/device_interface/code/secdev_default.o platform/hisi/sysdev.o platform/client_ree/secureapi_ree.o firmware/bin/vdecfw_bin.o ../../imgvideo/rpc/sysbrg/src/vdecdd_server.o ../../decoder/vdec/secure_media/secure_mem/sysmem_api_secure_ion.o
