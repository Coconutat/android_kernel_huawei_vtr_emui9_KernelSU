#!/bin/bash

# 清理每次同步KernelSU产生的软链接
if [ -d drivers/kernelsu ];
then
    echo "Find softlink kernelsu,will remove it"
	rm -rf drivers/kernelsu
	exit 0
else
	echo "No softlink kernelsu,good."
	exit 0
fi