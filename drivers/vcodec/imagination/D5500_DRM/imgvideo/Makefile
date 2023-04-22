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
# kernel makefile template for module imgvideo
#

KBUILD_CFLAGS := $(filter-out -Werror,$(KBUILD_CFLAGS))

obj-y := imgvideo.o

ccflags-y += -g -DIMG_MEMALLOC_UNIFIED_VMALLOC -DPORTFWRK_SYSMEM_ION -D__linux__ -D__PORT_FWRK__ -DSYSBRG_BRIDGING -DSECURE_TAL -DTARGET_SECURE_TAL -DTARGET_CONF_BY_HEADER_FILE -D__TALMMU_USE_SYSAPIS__ -D__TALMMU_NO_TAL__ -DIMG_KERNEL_MODULE
ccflags-y +=  -I$(src)/../imgvideo/platform/include/uapi -I$(src)/../imgvideo/platform/include -I$(src)/../imgvideo/port_fwrk/include -I$(src)/../imgvideo/system/include -I$(src)/../imgvideo/list_utils/include -I$(src)/../imgvideo/secure_media/target/include -I$(src)/../imgvideo/include -I$(src)/../imgvideo/include/linux-kernel -I$(src)/../imgvideo/rpc/sysbrg/src -I$(src)/../imgvideo/secure_media/tal/include -I$(src)/../imgvideo/kernel_comp/include -I$(src)/../imgvideo/osa/include -I$(src)/../imgvideo/imglib/libraries/listman/include -I$(src)/../imgvideo/imglib/libraries/talmmu_api/include -I$(src)/../imgvideo/imglib/libraries/cbman/include -I$(src)/../imgvideo/imglib/libraries/pixelapi/include -I$(src)/../imgvideo/imglib/libraries/pixelapi/code

ifeq ($(wildcard drivers/staging/android/ion/ion.h),)
ccflags-y += -DIMG_KERNEL_ION_HEADER="<linux/ion.h>"
else
ccflags-y += -DIMG_KERNEL_ION_HEADER="<$(srctree)/drivers/staging/android/ion/ion.h>"
endif

ifneq ( ,$(findstring arm64,$(ARCH)))
ccflags-y += -fno-pic
endif

imgvideo-objs :=  port_fwrk/kernel/linux/sysmem_api_ion.o  port_fwrk/kernel/linux/sysmem_api_unified.o platform/ion_client.o port_fwrk/kernel/sysmem_utils.o port_fwrk/kernel/dbgevent_api.o port_fwrk/kernel/dbgevent_api_km.o port_fwrk/kernel/dbgopt_api.o port_fwrk/kernel/dbgopt_api_km.o port_fwrk/kernel/dman_api.o port_fwrk/kernel/dman_api_km.o port_fwrk/kernel/pman_api.o port_fwrk/kernel/pool_api.o port_fwrk/kernel/sysbrg_utils.o port_fwrk/kernel/sysdev_utils.o port_fwrk/kernel/sysenv_utils.o port_fwrk/kernel/sysmem_utils.o port_fwrk/kernel/rman_api.o port_fwrk/kernel/idgen_api.o port_fwrk/kernel/page_alloc.o port_fwrk/kernel/sysenv_api.o port_fwrk/kernel/linux/sysos_api.o port_fwrk/kernel/linux/sysbrg_api.o port_fwrk/kernel/linux/sysbrg_drv.o port_fwrk/kernel/linux/perflog_api.o list_utils/src/dq/dq.o list_utils/src/lst/lst.o list_utils/src/trees/addchild.o list_utils/src/trees/addsib.o list_utils/src/trees/copysub.o list_utils/src/trees/finalsib.o list_utils/src/trees/firstchild.o list_utils/src/trees/init.o list_utils/src/trees/isinsub.o list_utils/src/trees/nextsib.o list_utils/src/trees/prevsib.o list_utils/src/trees/remleaf.o list_utils/src/trees/remlsub.o list_utils/src/trees/remsub.o list_utils/src/trees/subnext.o secure_media/tal/code/tal.o secure_media/target/code/target.o kernel_comp/libraries/wrap_utils/code/wrap_utils.o imglib/libraries/talmmu_api/code/talmmu_api.o imglib/libraries/talmmu_api/code/addr_alloc1.o imglib/libraries/talmmu_api/code/hash.o imglib/libraries/talmmu_api/code/pool.o imglib/libraries/talmmu_api/code/ra.o imglib/libraries/talmmu_api/code/trace.o imglib/libraries/pixelapi/code/pixel_api.o imglib/libraries/pixelapi/code/pixel_api_internals.o rpc/sysbrg/src/dbgevent_api_server.o rpc/sysbrg/src/dman_api_server.o rpc/sysbrg/src/dbgopt_api_server.o rpc/sysbrg/src/page_alloc_server.o rpc/sysbrg/src/wrap_utils_server.o
