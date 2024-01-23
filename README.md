
# No longer archived, but only for synchronizing KernelSU code.The purpose is only to allow friends in need to download the kernel and experience it.  
> Please download within Github Action.  
  
# 不再归档，但是仅仅只是为了同步KernelSU代码。目的只是让有需要的朋友可以下载内核体验。
> 请于Github Action内下载。  
***
# The End of Road
I declare with sadness that due to a job change, I will not have time to address the vast majority of related issues related to this project. I don't have time to study or improve either. Based on this, I have to archive this warehouse. Unfortunately, there are still many bugs in this project. But I am powerless. This has to be said to be a regret.  
# 路的尽头
我怀着悲痛的心情宣告以下内容，因为工作变动，我将没有时间处理绝大多数此项目的相关问题。我也没有时间研究或者改进。基于此，我不得不存档此仓库。很遗憾，这个项目仍有许多bug。但我已经无能为力。这不得不说，是一种遗憾。  
***  
  
# 华为P10支持KernelSU自定义内核EMUI 9版本  
[![Build Huawei-hi3660-KSU-Kernel](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-kernel.yml/badge.svg)](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions/workflows/build-huawei-hi3660-KSU-kernel.yml)
[下载统计](https://gra.caldis.me/?url=https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU)
   
***
## 重大改进：  
现已修复KernelSU对低于安卓10设备的兼容性。不再强制要求SElinux为宽容。   
感谢@longhuan1999   
***
**详细的说明(强烈建议先去阅读此文件！！)：[旧版README](README_OLD.md)**  
**此版本仅支持EMUI 9，支持EMUI 9.1.0的在此:[android_kernel_huawei_hi3660_emui9.1.0_KernelSU](https://github.com/Coconutat/android_kernel_huawei_hi3660_emui9.1.0_KernelSU)**  
***
# Github Action说明：
现在编译内核依托于Github Action进行全自动编译。编译均为KernelSU的开发版。  
Releases内会发布基于KernelSU稳定版本构建的内核。  
喜欢尝鲜的朋友可以在Action内下载。  
**版本说明：**
+ Build Huawei-hi3660-KSU-Kernel:给EMUI 9 和 GSI系统使用的KernelSU内核。  
 > 内部包含两个系列，一个是P10系列(Pangu_P10_KSU_XXXX)，一个是V9系列(Pangu_V9_KSU_XXXX)。解压后带enforcing的版本刷入后开机SELinux为强制模式。带permissive的版本刷入后开机SELinux为宽容模式。  

 + P10系列支持的机型：P10，P10 Plus，Mate9，Mate9 Pro  
 + V9系列支持的机型：荣耀荣耀9、v9（8Pro）、华为Nova2S、平板M5 8.4英寸、华为平板M5 10.8英寸（麒麟960s）
***  

# 下载：  
**稳定版：[Github Release](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/releases)**
**开发版：[Github Action](https://github.com/Coconutat/android_kernel_huawei_vtr_emui9_KernelSU/actions)**  

***
# 额外文档
1. 关于刷机的一些教程:[Wiki](https://github.com/Coconutat/HuaweiP10-GSI-And-Modify-Tutorial/wiki)  
2. 适配华为EMUI9/9.1.0内核的教程:[Wiki](https://github.com/Coconutat/HuaweiP10-GSI-And-Modify-Or-Support-KernelSU-Tutorial/wiki/7.KernelSU%E9%80%82%E9%85%8DEMUI9%E6%88%969.1.0%E7%B3%BB%E7%BB%9F%E7%9A%84%E5%86%85%E6%A0%B8)  

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

***
#### 滑稽  
![alt 术哥评价适配华为内核行为](https://s1.ax1x.com/2023/03/29/ppgmvo4.png)
