# 华为P10支持KernelSU自定义内核EMUI 9版本  
> 也可能支持：华为P10 Plus、Mate9、Mate9 Pro(VTR系列)  
## 目前增加了盘古内核支持的另一系列机型  
> 可能支持：荣耀荣耀9、v9（8Pro）、华为Nova2S、平板M5 8.4英寸、华为平板M5 10.8英寸（麒麟960s）(V9系列)  
> 下载地址：[蓝奏云](https://wwcy.lanzoum.com/b057ndkuj)  
> 访问密码：9awf  
> 此版本不会长期更新。  
> 机型支持来自盘古内核的说明：[说明](https://gitee.com/maimaiguanfan/Pangu9.1EROFS)  
***
## EMUI 8 版本(仅支持华为P10)：  
[android_kernel_huawei_vtr_emui8_KernelSU](https://github.com/Coconutat/android_kernel_huawei_vtr_emui8_KernelSU)  
***
  
## 关于此分支(GSI_upstream_dev)  
此分支是给那些希望在GSI系统上使用KernelSU的用户/开发者/编译者使用的。  
此分支下内核可以正常使用，SELinux状态可以自由切换。   
如果想修复一定的GSI系统的bug和性能问题，建议使用[huawei-creator](https://github.com/Iceows/huawei-creator)转换你的GSI，强烈推荐。  
   > 个人自用版huawei-creator:[huawei-creator-exp](https://github.com/Coconutat/huawei-creator-exp)   
***  
  
#### 关于刷机的一些教程
[Wiki](https://github.com/Coconutat/HuaweiP10-GSI-And-Modify-Tutorial/wiki)
***
# 特性：
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
 + 刷入TWRP：`fastboot flash recovery_ramdisk huawei-vtr-al00-em9_0-twrp3.2.3-7to-recovery-9.4.2.img` 
 进入TWRP，进入 **高级** ，选择 **移除DATA强制加密** ，刷入后，进入 **重启** ，**Recovery** ，之后重启进入TWRP后选择 **清除** ，**格式化DATA分区** ，格式化以后选择 **滑动恢复默认出厂**。  
 + 重启进入fastboot模式
 + 刷入内核：`fastboot flash kernel PK_VXXXX_9.0_P10_PM.img`  
 > XXXX是版本号。
 + 重启手机即可。
***   
# 自行构建：  
需求：  
 + Ubuntu 16.04 x86_64 / Ubuntu 20.04 x86_64  
 > 注：Ubuntu 20.04 需要Python2，并软连接成Python。  
 + 8GB RAM[最低] / 16GB RAM[推荐]
 + 64GB 或更多 硬盘空间
 + 克隆本仓库，android_kernel_huawei_hi3660 文件夹是内核，~~kernelsu_mod是是适配过此内核的KernelSU的部分源码~~(下面会解释，此处现在不需要了)。交叉编译器下载地址：[gcc-linaro-4.9.4-2017.01-x86_64_aarch64-elf](https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/aarch64-elf/gcc-linaro-4.9.4-2017.01-x86_64_aarch64-elf.tar.xz)，克隆命令：
 > `git clone https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU.git`  
 + 安装依赖：
 > `sudo apt install bc bison build-essential ccache curl flex g++-multilib gcc-multilib git git-lfs gnupg gperf imagemagick lib32ncurses5-dev lib32readline-dev lib32z1-dev libelf-dev liblz4-tool libncurses5 libncurses5-dev libsdl1.2-dev libssl-dev libxml2 libxml2-utils lzop pngcrush rsync schedtool squashfs-tools xsltproc zip zlib1g-dev libwxgtk3.0-dev adb fastboot`  
 > 注：Ubuntu 20.04 不需要libwxgtk3.0-dev。
 + 进入android_kernel_huawei_hi3660，运行命令：  
 > `bash synckernelsu.sh` （同步KernelSU最新代码，同步的分支是：主线，开发版。）
 + ~~运行命令 `bash modkernelsu.sh` （复制修改的KernelSU代码，为了适配此内核。）~~
 > 已经是过去式了，此PR已经改善了这一点[#374](https://github.com/tiann/KernelSU/commit/7be61b9657bfa257da926b8b86dfe84c435cacd0)
 + 在`Build_KSU_VTR.sh`脚本或者`Build_KSU_V9.sh`里填写好交叉编译器的路径。(内有注释。脚本使用取决于你编译什么机型系列。机型系列请看开头。)
 + 开始编译，命令：`bash Build_KSU.sh`
***
# 缺点/求助，如果能有大佬对这些问题有能力修正，请不吝赐教，感激不尽。
 + 不幸的是，这个内核不能切换SELinux的工作状态。如果切换就会导致KernelSU失效。所以我修改了/security/selinux/selinuxfs.c，在165行到167行添加了一些代码。
 + ~~不支持模块，目前模块功能刷入无效。~~   
> Alpha_1.1_KSU_0.4.1+_2023.03.30版本已经修复。  
> Alpha_1.4.2 修复了ZygiksOnKernelSU无法挂载的问题。感谢KernelSU团队。
***
# 更改记录：
 + /security/selinux/selinuxfs.c 165行到167行。  
 + ~~/driver/kernelsu/selinux/sepolicy.c 注释有Modify For Huawei的部分。~~  
 > 根据对比代码发现，华为4.9内核里的SELinux代码是移植自5.x版本的内核。所以修改了KernelSU关于版本检查的部分。
 > 已经是过去式了，此PR已经改善了这一点[#374](https://github.com/tiann/KernelSU/commit/7be61b9657bfa257da926b8b86dfe84c435cacd0)
 + 按照[KernelSU为非GKI集成教程](https://kernelsu.org/zh_CN/guide/how-to-integrate-for-non-gki.html)的部分。KPROBES能编译但是工作不正常。所以按照之后的添加代码方式移植。  
***
# 创建者/贡献者： 
 + [麦麦观饭](https://github.com/maimaiguanfan) / [麒麟盘古内核](https://github.com/maimaiguanfan/android_kernel_huawei_hi3660/)：提供了内核参考以及基础的内核。 
 + [kindle4jerry](https://github.com/kindle4jerry) : 感谢大佬的建议和无私帮助。  
 + [aaron74xda](https://github.com/aaron74xda) / [android_kernel_huawei_hi3660
](https://github.com/aaron74xda/android_kernel_huawei_hi3660):启发了我对于华为内核的强制SElinux宽容的具体思路。
 + [OnlyTomInSecond](https://github.com/OnlyTomInSecond) / [android_kernel_xiaomi_sdm845](https://github.com/OnlyTomInSecond/android_kernel_xiaomi_sdm845):提供了KernelSU的移植思路。  
 + [Aquarius223](https://github.com/Aquarius223) / [android_kernel_xiaomi_msm8998-ksu](https://github.com/sticpaper/android_kernel_xiaomi_msm8998-ksu)：修改SElinux的hook.c实现模块功能(可能吧)。  
 + [术哥](https://github.com/tiann) / [KernelSU](https://github.com/tiann)：开发了牛逼闪闪的各种炫酷东东的大佬。没有他就没有KernelSU。感谢他在我折腾华为内核期间给予的帮助。  


#### 滑稽  
![alt 术哥评价适配华为内核行为](https://s1.ax1x.com/2023/03/29/ppgmvo4.png)
