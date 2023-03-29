#
#NoC Makefile
#

subdir-ccflags-$(CONFIG_HISI_BB)               += -Idrivers/hisi/mntn/blackbox/
#subdir-ccflags-$(CONFIG_HISI_NOC)              += -Idrivers/hisi/mntn/ -Wall -Werror
subdir-ccflags-$(CONFIG_HISI_NOC)              += -Idrivers/hisi/mntn/ -Wall

obj-$(CONFIG_HISI_NOC) +=  hisi_noc.o             \
                           hisi_noc_err_probe.o       \
                           hisi_noc_packet.o      \
                           hisi_noc_transcation.o \
                           hisi_noc_info.o        \
                           hisi_noc_dump.o        \
                           hisi_noc_info_hi3650.o \
                           hisi_noc_info_hi3660.o \
                           hisi_noc_info_hi6250.o \
                           hisi_noc_info_kirin970_es.o \
                           hisi_noc_info_kirin970.o \
                           hisi_noc_info_MIA.o \
                           hisi_noc_info_ATLA_es.o \
                           hisi_noc_info_ATLA.o \
                           hisi_noc_info_ORLA.o \
                           hisi_noc_info_PHOE_es.o \
                           hisi_noc_info_PHOE.o \

obj-$(CONFIG_HISI_NOC_DBG) += hisi_noc_dbg.o

