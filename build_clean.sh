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

# 注意：这是 KernelSU-Next 自己的逻辑问题，不是内核缺少 hook 代码。
#
# drivers/kernelsu/Kbuild 用 CONFIG_KSU_* 来判断用哪种 hook 模式：
#   CONFIG_KSU_KPROBES_HOOK=y       -> 认为有 hook
#   CONFIG_KSU_SYSCALL_TABLE_HOOK=y -> 认为有 hook
#   CONFIG_KSU_MANUAL_HOOK=y        -> 去 kernel/reboot.c 里 grep ksu_handle_sys_reboot
# 三个都不成立时，HAVE_KSU_HOOK 保持初始值 1，于是直接 $(error) "No hooks were defined"。
#
# 而 distclean 时树里通常没有 .config，CONFIG_KSU_* 全是空值，三个 ifeq 一个都不进，
# 那句 grep 根本不会执行 —— 所以哪怕 reboot.c 里明明有 ksu_handle_sys_reboot，
# 清理也会失败。这里显式传入 CONFIG_KSU_MANUAL_HOOK=y 让判定能走到 grep。
# 本内核 CONFIG_KPROBES 是关的，KPROBES 模式不可用，manual 是唯一可行项。
#
# 副作用：判定通过后 Kbuild 会用 sed 往内核源码注入 backport（can_umount/path_umount、
# seccomp filter_count、selinux_inode/selinux_cred 等），这是 KernelSU-Next 的设计，
# 注入带 grep 守卫、可重复执行，不要在清理后把这些改动当成脏数据删掉。
make ARCH=arm64 CONFIG_KSU_MANUAL_HOOK=y HOSTCC="${HOSTCC}" HOSTCXX="${HOSTCXX}" PYTHON="${PYTHON}" distclean
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