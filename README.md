# 华为P10支持KernelSU自定义内核EMUI 9版本  
***
## EMUI 8 版本：  
[android_kernel_huawei_vtr_KernelSU](https://github.com/Coconutat/android_kernel_huawei_vtr_KernelSU)  
***
特性：
 1. 基于原本的Pangu Kernel(盘古内核)的所有特性。
 2. 支持KernelSU
 3. 不能切换SELinux为强制模式。(强制为宽容模式。)
 4. 完全屏蔽华为内核级别ROOT检查和扫描。  
***  
# 下载：  
[Github Release](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/releases/)  
包含TWRP，自定义内核和原始内核。  
***  
# 刷写：  
 1. 刷入TWRP：`fastboot flash recovery_ramdisk huawei-vtr-al00-em9_0-twrp3.2.3-7to-recovery-9.4.2.img` 
 2. 进入TWRP，进入 **高级** ，选择 **移除DATA强制加密** ，刷入后，进入 **重启** ，**Recovery** ，之后重启进入TWRP后选择 **清除** ，**格式化DATA分区** ，格式化以后选择 **滑动恢复默认出厂**。  
 3. 重启进入fastboot模式
 4. 刷入内核：`fastboot flash kernel PK_VXXXX_9.0_P10_PM.img`  
 > XXXX是版本号。
 5. 重启手机即可。
***   
# 自行构建：  
需求：  
 1. Ubuntu 16.04 x86_64 / Ubuntu 20.04 x86_64  
 > 注：Ubuntu 20.04 需要Python2，并软连接成Python。  
 1. 8GB RAM[最低] / 16GB RAM[推荐]
 2. 64GB 或更多 硬盘空间
 3. 克隆本仓库，android_kernel_huawei_hi3660 文件夹是内核，交叉编译器下载地址：[gcc-linaro-4.9.4-2017.01-x86_64_aarch64-elf](https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/aarch64-elf/gcc-linaro-4.9.4-2017.01-x86_64_aarch64-elf.tar.xz)，克隆命令：
 > `git clone https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU.git`  
 1. 安装依赖：
 > `sudo apt install bc bison build-essential ccache curl flex g++-multilib gcc-multilib git git-lfs gnupg gperf imagemagick lib32ncurses5-dev lib32readline-dev lib32z1-dev libelf-dev liblz4-tool libncurses5 libncurses5-dev libsdl1.2-dev libssl-dev libxml2 libxml2-utils lzop pngcrush rsync schedtool squashfs-tools xsltproc zip zlib1g-dev libwxgtk3.0-dev adb fastboot`  
 > 注：Ubuntu 20.04 不需要libwxgtk3.0-dev。
 1. 在`Build_KSU.sh`脚本里填写好交叉编译器的路径。(内有注释。)
 2. 开始编译，命令：`bash Build_KSU.sh`
***
# 缺点/求助，如果能有大佬对这些问题有能力修正，请不吝赐教，感激不尽。
1. 不幸的是，这个内核不能切换SELinux的工作状态。如果切换就会导致KernelSU失效。所以我修改了/security/selinux/selinuxfs.c，在165行到167行添加了一些代码。
***
# 更改记录：
 1. /security/selinux/selinuxfs.c 165行到167行。  
 2. /driver/kernel/selinux/sepolicy.c 注释有Modify For Huawei的部分。  
 3. 按照[KernelSU为非GKI集成教程](https://kernelsu.org/zh_CN/guide/how-to-integrate-for-non-gki.html)的部分。KPROBES能编译但是工作不正常。所以按照之后的添加代码方式移植。  
***
# 创建者/贡献者： 
[麦麦观饭](https://github.com/maimaiguanfan) / [麒麟盘古内核](https://github.com/maimaiguanfan/android_kernel_huawei_hi3660/)：提供了内核参考以及基础的内核。  
[aaron74xda](https://github.com/aaron74xda) / [android_kernel_huawei_hi3660
](https://github.com/aaron74xda/android_kernel_huawei_hi3660):启发了我对于华为内核的强制SElinux宽容的具体思路。
[OnlyTomInSecond](https://github.com/OnlyTomInSecond) / [android_kernel_xiaomi_sdm845](https://github.com/OnlyTomInSecond/android_kernel_xiaomi_sdm845):提供了KernelSU的移植思路。  
[术哥](https://github.com/tiann) / [KernelSU](https://github.com/tiann)：开发了牛逼闪闪的各种炫酷东东的大佬。没有他就没有KernelSU。感谢他在我折腾华为内核期间给予的帮助。  


#### 滑稽  
![alt 术哥评价适配华为内核行为](https://imgse.com/i/ppgmvo4 "暴论")
