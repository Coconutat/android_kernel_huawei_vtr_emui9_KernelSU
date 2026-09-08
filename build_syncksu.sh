#!/bin/bash

# 0. 检查必要的依赖
if ! command -v curl &> /dev/null; then
    echo "Error: curl is not installed. Please install it first."
    exit 1
fi

# 1. 简单的环境检查 (可选，确保在内核源码根目录)
if [ ! -f "Makefile" ]; then
    echo "Warning: No Makefile found. Are you in the kernel source root directory?"
    read -p "Press Enter to continue anyway, or Ctrl+C to abort..."
fi

# 2. 清理旧文件
if [ -d "KernelSU" ]; then
    echo "Found KernelSU Folder, removing it..."
    rm -rf KernelSU
    # 同时也清理驱动目录下的软链接或文件夹，防止冲突
    rm -rf drivers/kernelsu
else
    echo "KernelSU Folder not found, proceeding..."
fi

# 3. 获取用户输入
echo "-------------------------------------------------------"
echo "Please select the KernelSU version to install:"
echo " A) Tags version (rsuntk)"
echo " B) SukiSU Builtin (SukiSU-Ultra)"
echo " C) Main version (rsuntk)"
echo " D) Master version (backslashxx)"
echo " E) ReSukiSU Builtin (ReSukiSU)"
echo "-------------------------------------------------------"
read -p "Enter your choice (A/B/C/D/E): " version

if [ -z "$version" ]; then
    echo "No version specified. Exiting."
    exit 1
fi

# 定义通用的 curl 选项: 
# -f (fail silently on HTTP errors)
# -L (follow redirects)
# -S (show error)
# -s (silent mode)
CURL_OPTS="-fLSs"

# 4. 执行下载与安装
case $version in
    [Aa]*)
        echo "Installing rsuntk KernelSU (Tags)..."
        curl $CURL_OPTS "https://raw.githubusercontent.com/rsuntk/KernelSU/main/kernel/setup.sh" | bash -s tags
        ;;
    [Bb]*)
        echo "Installing SukiSU-Ultra (Builtin)..."
        curl $CURL_OPTS "https://raw.githubusercontent.com/SukiSU-Ultra/SukiSU-Ultra/main/kernel/setup.sh" | bash -s builtin
        ;;
    [Cc]*)
        echo "Installing rsuntk KernelSU (Main)..."
        curl $CURL_OPTS "https://raw.githubusercontent.com/rsuntk/KernelSU/main/kernel/setup.sh" | bash -s main
        ;;
    [Dd]*)
        echo "Installing backslashxx KernelSU (Master)..."
        curl $CURL_OPTS "https://raw.githubusercontent.com/backslashxx/KernelSU/master/kernel/setup.sh" | bash -s master
        ;;
    [Ee]*)
        echo "Installing ReSukiSU (Builtin)..."
        curl $CURL_OPTS "https://raw.githubusercontent.com/ReSukiSU/ReSukiSU/main/kernel/setup.sh" | bash -s builtin
        ;;
    *)
        echo "Invalid option: $version. Exiting."
        exit 1
        ;;
esac

# 检查上一步管道命令的退出状态 (检查 bash -s 的执行结果)
if [ $? -eq 0 ]; then
    echo "Setup script finished successfully."
else
    echo "Error: Setup script failed or network issue occurred."
    exit 1
fi