# 华为P10支持KernelSU自定义内核EMUI 9版本  
[![Build Huawei-hi3660-KSU-EMUI9-EPM-Kernel](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-EMUI9-EPM-kernel.yml/badge.svg?branch=Github_Action_Mode)](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-EMUI9-EPM-kernel.yml) 
[![Build Huawei-hi3660-KSU-GSI-Kernel](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-GSI-kernel.yml/badge.svg?branch=Github_Action_Mode)](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-GSI-kernel.yml) ![Downloads](https://img.shields.io/github/downloads/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/total)  

[下载统计](https://gra.caldis.me/?url=https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU)
   
***
**详细的说明(强烈建议先去阅读此文件！！)：[旧版README](README_OLD.md)**  

***
# Github Action说明：
现在编译内核依托于Github Action进行全自动编译。编译均为KernelSU的开发版。  
Releases内会发布基于KernelSU稳定版本构建的内核。  
喜欢尝鲜的朋友可以在Action内下载。  
**版本说明：**
+ Build Huawei-hi3660-KSU-GSI-Kernel:给GSI系统使用的KernelSU内核。  
 > 内部包含两个系列，一个是P10系列(Pangu_P10_KSU_XXXX)，一个是V9系列(Pangu_V9_KSU_XXXX)。解压后带enforcing的版本刷入后开机SELinux为强制模式。带permissive的版本刷入后开机SELinux为宽容模式。  
  
+ Build Huawei-hi3660-KSU-EMUI9-EPM-Kernel:给华为EMUI9.0.x系列系统使用的KernelSU内核。  
 > 内部包含两个系列，一个是P10系列(Pangu_P10_KSU_XXXX)，一个是V9系列(Pangu_V9_KSU_XXXX)。解压后无论是enforcing还是permissive都会强制SELinux为宽容模式。(不然KernelSU会失效。)  

 + P10系列支持的机型：P10，P10 Plus，Mate9，Mate9 Pro  
 + V9系列支持的机型：荣耀荣耀9、v9（8Pro）、华为Nova2S、平板M5 8.4英寸、华为平板M5 10.8英寸（麒麟960s）
***  

# 下载：  
**稳定版：[Github Release](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/releases)**
**开发版：[Github Action](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions)**  

***  
# 创建者/贡献者： 
 + [麦麦观饭](https://github.com/maimaiguanfan) / [麒麟盘古内核](https://github.com/maimaiguanfan/android_kernel_huawei_hi3660/)：提供了内核参考以及基础的内核。 
 + [kindle4jerry](https://github.com/kindle4jerry) : 感谢大佬的建议和无私帮助。  
 + [aaron74xda](https://github.com/aaron74xda) / [android_kernel_huawei_hi3660
](https://github.com/aaron74xda/android_kernel_huawei_hi3660):启发了我对于华为内核的强制SElinux宽容的具体思路。
 + [OnlyTomInSecond](https://github.com/OnlyTomInSecond) / [android_kernel_xiaomi_sdm845](https://github.com/OnlyTomInSecond/android_kernel_xiaomi_sdm845):提供了KernelSU的移植思路。  
 + [Aquarius223](https://github.com/Aquarius223) / [android_kernel_xiaomi_msm8998-ksu](https://github.com/sticpaper/android_kernel_xiaomi_msm8998-ksu)：修改SElinux的hook.c实现模块功能(可能吧)。  
 + [术哥](https://github.com/tiann) / [KernelSU](https://github.com/tiann)：开发了牛逼闪闪的各种炫酷东东的大佬。没有他就没有KernelSU。感谢他在我折腾华为内核期间给予的帮助。  
 + [lateautumn233](https://github.com/lateautumn233) / [android_kernel_oneplus_sm8250](https://github.com/lateautumn233/android_kernel_oneplus_sm8250)：启发我使用Github Action编译内核。(解决了我外地上班只有手机的痛点。)


#### 滑稽  
![alt 术哥评价适配华为内核行为](https://s1.ax1x.com/2023/03/29/ppgmvo4.png)
