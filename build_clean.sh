#!/bin/bash

# 开始运行
echo "开始清理编译残留..."
start_time_sum=$(date +%s)

rm -rf out

# 编译命令定义（系统装了 python env，python 默认指向 py3，无法切回 python2）
# 内核本体用 CROSS_COMPILE 交叉编译器，这里约束的是主机侧工具链
export CC=gcc-9
export CXX=g++-9
export HOSTCC=gcc-9
export HOSTCXX=g++-9
export PYTHON=python2


export PATH=$PATH:$(pwd)/../Compiler/aarch64-linux-android-4.9-pie/bin
export CROSS_COMPILE=aarch64-linux-android-

make ARCH=arm64 HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" distclean
# rm -rf KernelSU
# git reset --hard

end_time_sum=$(date +%s)

# 计算运行时间（秒）
duration=$((end_time_sum - start_time_sum))

# 将秒数转化为 "小时:分钟:秒" 形式输出
hours=$((duration / 3600))
minutes=$(( (duration % 3600) / 60 ))
seconds=$((duration % 60))

# 打印运行时间
echo "脚本运行时间为：${hours}小时 ${minutes}分钟 ${seconds}秒"