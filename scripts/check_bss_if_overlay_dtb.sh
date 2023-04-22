#!/bin/bash
set -e

if [ "$1" == "" -o "$2" == "" -o "$3" == "" ];
then
	echo "Usage:";
	echo "       check_bss_if_overlay_dtb.sh <KERNEL_ADDRESS> <DTB_ADDRESS> <vmlinux>";
	exit 0;
fi

KERNEL_ADDRESS=$1
DTB_ADDRESS=$2

vmlinux_end=`nm $3 | grep -E "\<_end\>" | cut -d' ' -f1`
vmlinux_end=`echo "0x${vmlinux_end:8:16}"`

DTB_HEX_ADDRESS=$((16#${DTB_ADDRESS/0x/}))
VME_HEX_ADDRESS=$[vmlinux_end - 0x08000000 - KERNEL_ADDRESS]

if [ ${VME_HEX_ADDRESS} -ge ${DTB_HEX_ADDRESS} ];
then
	echo "BSS section(${VME_HEX_ADDRESS}) and dtb(${DTB_HEX_ADDRESS}) range is overlayed,please fix it!";
	exit -1;
fi
