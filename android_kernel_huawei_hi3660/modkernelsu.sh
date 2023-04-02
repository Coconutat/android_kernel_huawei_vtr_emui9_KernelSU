#!/bin/bash

# 修改KenrelSU的代码以适配内核
KERNELSU_DIR=$(pwd)/KernelSU
echo "[+] KERNELSU_DIR: $KERNELSU_DIR"



if [ -d ../kernelsu_mod ];
then
    echo "Find kernelsu_mod floder,will copy it"
	cp -r  ../kernelsu_mod/selinux $KERNELSU_DIR/kernel
else
	echo "No kernelsu_mod floder,good."
fi