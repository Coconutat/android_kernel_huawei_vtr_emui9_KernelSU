# 麒麟960盘古内核
===

### 内容来源均来自原作者麦麦观饭

已搬迁至https://github.com/maimaiguanfan/android_kernel_huawei_hi3660


特性
---
解锁selinux状态限制，可调为permissive

解锁官方隐藏的CPU调度器Schedutil

移植[荣耀9 EMUI8 Proto内核](https://github.com/JBolho/Proto)的CPU调度器Blu_Schedutil，并设为默认

添加Dynamic Stune Boost

添加WireGuard

新增I/O调度器ZEN，并设为默认

Upstream至Linux4.9.155

移植970的JPEG Processing引擎

fsync开关

支持的设备
---
 **爵士定制版:**  华为P10、P10 Plus、Mate9、Mate9 Pro

 **骑士定制版:**  荣耀9、v9（8Pro）、华为Nova2S、平板M5 8.4英寸、华为平板M5 10.8英寸（麒麟960s）

支持的系统
---
 支持EMUI9所有版本、以及基于这些版本刷入的类原生ROM。

 请根据你的系统或底包版本选择刷入9.0或9.1版本

关于版本的说明
---
每个版本有12个文件，9.0和9.1各6个，其中，爵士和骑士又各有3个

3个文件中，一个是.zip，两个个是.img

zip是卡刷包，用[Anykernel3](https://github.com/maimaiguanfan/AnyKernel3)打包，适合第三方rec刷入

img是镜像文件，可以fastboot刷，也可以rec刷，刷到kernel分区

img中文件名带PM的SELinux状态默认为permissive模式，又称SELinux宽容模式（部分类原生需要permissive才能开机，如果你看不懂就别刷这个）

卡刷zip会保持上一个内核的默认SeLinux状态

 **前方高能！！！** 

编译教程
===
第零步
---
Linux环境（Windows子系统也可以）

熟悉Linux终端操作

学习git知识


第一步：下载
---
克隆源码到本地

克隆或下载[GCC 4.9](https://gitee.com/maimaiguanfan/android-gcc)到本地

另外还有[GCC 9.2](https://gitee.com/maimaiguanfan/arm-gcc)可以使用，后面步骤用的就是[build_gcc9.2.sh](https://github.com/maimaiguanfan/android_kernel_huawei_hi3660/blob/master/build_gcc9.2.sh)

第二步：配置
---
打开终端，安装` zip make python2 python-is-python2 `

第三部：编译
---
cd到源码路径，运行`sh build.sh`（如果你用的是WIndows子系统，请把`export CROSS_COMPILE`那一行注释掉，在命令行里单独运行，因为Windows的环境变量会影响WSL）

编译过程可能会卡住，那就按一下回车

如果出错，那就再运行一次`sh build.sh`，不要怕，断点可以继续

如果编译成功，输出的文件都在源码目录里

鸣谢：
===
[ **kindle4jerry大佬** ](http://github.com/kindle4jerry)，提供指引，帮我修复编译错误

[ **JBolho大佬** ](http://github.com/JBolho)，他的Proto内核为我提供了大量帮助

[ **engstk大佬** ](https://github.com/engstk)，他的[荣耀v10 Blu_Spark内核](https://github.com/engstk/view10)为我提供了大量帮助

[ **joshuous大佬** ](http://github.com/joshuous/)，提供Dynamic Stune Boost源码

还有很多位测试人员以及提供帮助和建议的人
