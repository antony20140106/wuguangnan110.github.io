# README

以高通平台qcm2290为例，分析一下QSSI是什么。

# refers


# 分析

* 1. QSSI 是 Qualcomm Single System Image 的缩写。
* 2. Android Q上开始支持QSSI。
* 3. QSSI 是用来编译system.img的
  * 3.1 QSSI编译注意事项
    * lunch qssi ------ 编译system.img
    * lunch target ------ 编译其余的image
  * 3.2 有QSSI和没有QSSI的编译流程对比

# 没有QSSI

```shell
source build/envsetup.sh
lunch <target>
make
```

# 有QSSI编译system.img

```shell
source build/envsetup.sh
lunch qssi
make <system related images>
#编译其他 img
source build/envsetup.sh
lunch <target>
make <non-system images>
```

# 为什么要用QSSI
解决Android碎片化问题，把system.img和vendor.img进一步拆分
