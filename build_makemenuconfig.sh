#!/bin/bash
#设置环境

# 编译命令定义（系统装了 python env，python 默认指向 py3，无法切回 python2）
# 内核本体用 CROSS_COMPILE 交叉编译器，这里约束的是主机侧工具链
export CC=gcc-9
export CXX=g++-9
export HOSTCC=gcc-9
export HOSTCXX=g++-9
export PYTHON=python2

# 交叉编译器路径
export PATH=$PATH:$(pwd)/../Compiler/aarch64-linux-android-4.9-pie/bin
export CROSS_COMPILE=aarch64-linux-android-

export ARCH=arm64
# export DTC_EXT=dtc

make ARCH=arm64 HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" O=out Pangu_P10_KSU_defconfig
make ARCH=arm64 HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" O=out menuconfig