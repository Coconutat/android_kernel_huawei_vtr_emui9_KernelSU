#!/bin/bash
#设置环境

# Special Clean For Huawei Kernel.
if [ -d include/config ];
then
    echo "Find config,will remove it"
	rm -rf include/config
else
	echo "No Config,good."
fi


echo " "
echo "***Setting environment...***"
# 交叉编译器路径
export PATH=$PATH:
export CROSS_COMPILE=aarch64-elf-

export GCC_COLORS=auto
export ARCH=arm64
if [ ! -d "out" ];
then
	mkdir out
fi

#添加或更新AK3

if [ -f tools/AnyKernel3/README.md ];
then
	cd tools/AnyKernel3
	echo " "
	echo "***Updating AnyKernel3...***"
	echo " "
	git pull upstream master
else
	echo " "
	echo "***Adding AnyKernel3...***"
	echo " "
	git submodule update --init --recursive
	cd tools/AnyKernel3
	git remote add upstream https://github.com/osm0sis/AnyKernel3
	echo "***Updating AnyKernel3...***"
	echo " "
	git pull upstream master
fi
cd ../..
echo " "

#输入盘古内核版本号
printf "Please enter Pangu Kernel version number: "
read v
echo " "
echo "Setting EXTRAVERSION"
export EV=EXTRAVERSION=_Kirin960_Pangu_KSU_V$v

#构建P10内核部分
echo "***Building for P10 version...***"
make ARCH=arm64 O=out $EV Pangu_P10_KSU_defconfig
# 定义编译线程数
make ARCH=arm64 O=out $EV -j256

#打包P10版内核

if [ -f out/arch/arm64/boot/Image.gz ];
then
	echo "***Packing P10 version kernel...***"
	tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=enforcing buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2020-09-05  --output PK_V"$v"_9.0_P10.img
	tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2020-09-05  --output PK_V"$v"_9.0_P10_PM.img
	cp out/arch/arm64/boot/Image.gz tools/AnyKernel3/Image.gz
	cp out/arch/arm64/boot/Image.gz Image.gz 
	cd tools/AnyKernel3
	zip -r9 PK_V"$v"_9.0_P10.zip * > /dev/null
	cd ../..
	mv tools/AnyKernel3/PK_V"$v"_9.0_P10.zip PK_V"$v"_9.0_P10.zip
	rm -rf tools/AnyKernel3/Image.gz
	echo " "
	echo "***Sucessfully built P10 version kernel...***"
	echo " "
	exit 0
else
	echo " "
	echo "***Failed!***"
	exit 0
fi