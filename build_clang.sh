#!/bin/bash
# build_clang.sh - 用 Clang 构建 Kirin 960 (VTR/P10, hi3660) 内核
#
# 用法:
#   ./build_clang.sh                 # 默认 Google/clang-r547379 (clang 20)
#   ./build_clang.sh <clang目录>      # 换工具链，例如 .../Zyc_Clang_16
#   ./build_clang.sh <clang目录> --clean
#
# 设计要点:
#   * 主机工具 (modpost/dtc/kconfig) 用本机 GCC，target 用交叉 clang —— 这就是
#     "本机 GCC + 交叉编译器" 的模型，不需要装任何 apt 交叉 GCC。
#   * 必须保留 GNU binutils 的 as/ld：树里有 65 个 directly-emitted 的 .S
#     (block/hisi-*.S, drivers/vcodec/*.S) 只有 GCC 汇编语法和 DWARF 指令，
#     LLVM 集成汇编器吃不消，所以走 Makefile 里已有的 -no-integrated-as。
#   * 输出目录与 GCC 构建分离 (out-clang)，互不干扰。

set -o pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$ROOT" || exit 1

CLANG_DIR="${1:-/mnt/sdb1/Github/Compiler/Google/clang-r547379}"
if [ "$1" = "--clean" ]; then CLANG_DIR="/mnt/sdb1/Github/Compiler/Google/clang-r547379"; fi
BINUTILS_DIR="/mnt/sdb1/Github/Compiler/aarch64-linux-android-4.9-pie/bin"
OUT=out-clang
JOBS=$(nproc)

if [ ! -x "$CLANG_DIR/bin/clang" ]; then
	echo "*** 找不到 clang: $CLANG_DIR/bin/clang"
	exit 1
fi
if [ ! -x "$BINUTILS_DIR/aarch64-linux-android-as" ]; then
	echo "*** 找不到 GNU as: $BINUTILS_DIR/aarch64-linux-android-as"
	exit 1
fi

export PATH="$CLANG_DIR/bin:$BINUTILS_DIR:$PATH"
export ARCH=arm64
export SUBARCH=arm64
export CROSS_COMPILE=aarch64-linux-android-
export CLANG_TRIPLE=aarch64-linux-android-
export HOSTCC=${HOSTCC:-gcc}
export HOSTCXX=${HOSTCXX:-g++}

echo "*** 工具链 ***"
"$CLANG_DIR/bin/clang" --version | head -1
"$BINUTILS_DIR/aarch64-linux-android-as" --version | head -1
echo "host gcc: $($HOSTCC --version | head -1)"
echo "output  : $OUT"
echo

if [ "$2" = "--clean" ] || [ "$1" = "--clean" ]; then
	echo "*** 清理 $OUT"
	rm -rf "$OUT"
fi
mkdir -p "$OUT"

start_time_sum=$(date +%s)
start_time=$(date +%Y.%m.%d-%I_%M)
LOG="kernel_log-clang-${start_time}.log"

echo "*** 配置 (Pangu_P10_KSU_defconfig) ***"
make O=$OUT ARCH=arm64 CC=clang Pangu_P10_KSU_defconfig || exit 1

# clang 专属调整：
#   GCC_PLUGINS - 插件在 clang 下本来就不会被加载（gcc-plugin.sh 探不到 plugin 目录），
#                 显式关掉可以避免 Kconfig 层误导。
#   DWARF4      - clang 16+ 默认吐 DWARF5，而 as/ld 是 binutils 2.27，
#                 不钉住会在链接阶段炸。
#   HISI_CLANG  - 打开 scripts/Makefile.build:195 里已有的 -Werror 过滤。
#                 供应商 Makefile 里几十个 subdir-ccflags-y += -Werror 是 GCC 4.9
#                 时代的，clang 新增的警告类别（-Wsometimes-uninitialized 等）
#                 会直接变成 error。
#   KPROBES     - 非 GKI 内核不开（保持 defconfig 的 "not set"）。注意
#                 drivers/kernelsu 的 `config KSU` 硬依赖 KPROBES，所以这轮
#                 镜像里 KSU 会被静默丢弃。
scripts/config --file $OUT/.config -d GCC_PLUGINS -e DEBUG_INFO_DWARF4 \
	-e HISI_CLANG -d KPROBES
make O=$OUT ARCH=arm64 CC=clang olddefconfig || exit 1
echo

echo "*** 编译 (-k，尽可能收集全部错误) ***"
make O=$OUT ARCH=arm64 CC=clang -j"$JOBS" -k 2>&1 | tee "$LOG"

end_time_sum=$(date +%s)
duration=$((end_time_sum - start_time_sum))
echo
echo "脚本运行时间为：$((duration / 3600))小时 $(((duration % 3600) / 60))分钟 $((duration % 60))秒"

if [ -f "$OUT/arch/arm64/boot/Image.gz" ]; then
	echo "*** 编译成功 ***"
	cp "$OUT/arch/arm64/boot/Image.gz" Image.gz
	tools/mkbootimg --kernel "$OUT/arch/arm64/boot/Image.gz" --base 0x0 \
		--cmdline "loglevel=7 initcall_debug=n page_tracker=on slub_min_objects=16 unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive buildvariant=user" \
		--tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 \
		--header_version 1 --os_version 9 --os_patch_level 2026-09-10 \
		--output Kirin960_9_Kernel_Clang.img
	echo "*** 已打包: Kirin960_9_Kernel_Clang.img (permissive) ***"
	ls -lh Kirin960_9_Kernel_Clang.img
	exit 0
else
	echo "*** 编译失败，见 $LOG ***"
	exit 1
fi
