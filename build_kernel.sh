#!/bin/bash
#设置环境

# GCC交叉编译器路径
export PATH=$PATH:$(pwd)/../Compiler/aarch64-linux-android-4.9-pie/bin
export CROSS_COMPILE=aarch64-linux-android-
export ARCH=arm64
export SUBARCH=arm64

# GCC颜色输出
export CFLAGS="-fdiagnostics-color=always"
export CXXFLAGS="-fdiagnostics-color=always"

# 编译命令定义（系统装了 python env，python 默认指向 py3，无法切回 python2）
# 内核本体用 CROSS_COMPILE 交叉编译器，这里约束的是主机侧工具链
export CC=gcc-9
export CXX=g++-9
export HOSTCC=gcc-9
export HOSTCXX=g++-9
export PYTHON=python2

# export DTC_EXT=dtc

if [ ! -d "out" ]; then
	mkdir out
fi

start_time=$(date +%Y.%m.%d-%I_%M)

start_time_sum=$(date +%s)

make ARCH=arm64 O=out HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" Pangu_P10_KSU_defconfig
# 定义编译线程数
make ARCH=arm64 O=out -j$(nproc --all) HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" 2>&1 | tee kernel_log-${start_time}.log

end_time_sum=$(date +%s)

end_time=$(date +%Y.%m.%d-%I_%M)

# 计算运行时间（秒）
duration=$((end_time_sum - start_time_sum))

# 将秒数转化为 "小时:分钟:秒" 形式输出
hours=$((duration / 3600))
minutes=$(((duration % 3600) / 60))
seconds=$((duration % 60))

# 打印运行时间
echo "脚本运行时间为：${hours}小时 ${minutes}分钟 ${seconds}秒"

if [ -f out/arch/arm64/boot/Image.gz ]; then

	echo "***Sucessfully built kernel...***"
	cp out/arch/arm64/boot/Image.gz Image.gz
	./tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=7 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=enforcing buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2026-11-01  --output Kirin960_9_Kernel.img
	./tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=7 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2026-11-01  --output Kirin960_9_Kernel_PM.img
	exit 0
else
	echo " "
	echo "***Failed!***"
	exit 0
fi
