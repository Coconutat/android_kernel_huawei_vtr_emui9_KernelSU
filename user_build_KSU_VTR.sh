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
export PATH=$PATH:$(pwd)/../ToolChains/bin
export CROSS_COMPILE=aarch64-linux-gnu-

export GCC_COLORS=auto
export ARCH=arm64
if [ ! -d "out" ];
then
	mkdir out
fi

# Choice a Kernel Version: 
echo "What version do you want to Compile ? EPM or GSI?"
echo "If EPM Say:Y"
echo "If GSI:Say:N"
# 提示用户输入
read -p "Copy file?(Y/N): " choice

# 判断用户的选择
if [ "$choice" = "Y" ] || [ "$choice" = "y" ]; then
    # 复制文件到文件夹
    cp -vf MOD_FILE/selinuxfs/selinuxfs.c security/selinux/selinuxfs.c
    echo "File has copyed。"
elif [ "$choice" = "N" ] || [ "$choice" = "n" ]; then
    echo "File doesn't copyed."
else
    echo "error!"
fi

#输入盘古内核版本号
printf "Please enter Pangu Kernel version number: "
read v
echo " "
echo "Setting EXTRAVERSION"
export EV=EXTRAVERSION=-KSU_V$v

start_time=$(date +%Y.%m.%d-%I_%M)

start_time_sum=$(date +%s)

#构建P10内核部分
echo "***Building for P10 version...***"
make ARCH=arm64 O=out $EV Pangu_P10_KSU_defconfig
# 定义编译线程数
make ARCH=arm64 O=out $EV -j256 2>&1 | tee kernel_log-${start_time}.txt

end_time_sum=$(date +%s)

end_time=$(date +%Y.%m.%d-%I_%M)

# 计算运行时间（秒）
duration=$((end_time_sum - start_time_sum))

# 将秒数转化为 "小时:分钟:秒" 形式输出
hours=$((duration / 3600))
minutes=$(( (duration % 3600) / 60 ))
seconds=$((duration % 60))

# 打印运行时间
echo "脚本运行时间为：${hours}小时 ${minutes}分钟 ${seconds}秒"

# Restore Original Version Selinuxfs file
cp -vf MOD_FILE/selinux_BAK/selinuxfs.c security/selinux/selinuxfs.c

#打包P10版内核

if [ -f out/arch/arm64/boot/Image.gz ];
then
	echo "***Packing P10 version kernel...***"

	cp out/arch/arm64/boot/Image.gz Image.gz 

	# Pack Enforcing Kernel
	tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=enforcing buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2020-09-05  --output PK_V"$v"_9.0_P10-${end_time}.img
	
	# Pack Permissive Kernel
	tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2020-09-05  --output PK_V"$v"_9.0_P10_PM-${end_time}.img

	echo "***Sucessfully built P10 version kernel...***"
	echo " "
	exit 0
else
	echo " "
	echo "***Failed!***"
	exit 0
fi