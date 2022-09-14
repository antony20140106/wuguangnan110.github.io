# README

以高通平台qcm2290为例，分析一下Kernel如何确定使用哪个defconfig文件。

# refers

* [Android Kernel如何确定使用哪个defconfig文件](https://blog.csdn.net/daoshuti/article/details/101841641)

# 答案

一般来说，`device/厂商名/项目名/AndroidBoard.mk`中`KERNEL_DEFCONFIG`变量决定了使用哪个def_config文件，而`TARGET_BUILD_VARIANT`变量是在Android编译系统中定义的这个变量决定了编译的是userdebug还是eng版本，比如：
```shell
qcom:
device/qcom/bengal_32/AndroidBoard.mk
31:ifeq ($(KERNEL_DEFCONFIG),)
33:          KERNEL_DEFCONFIG := vendor/bengal-perf_defconfig
35:          KERNEL_DEFCONFIG := vendor/bengal_defconfig

mtk:
device/mediateksample/k62v1_64_pax/vnd_k62v1_64_pax.mk
ifeq ($(TARGET_BUILD_VARIANT), eng)
KERNEL_DEFCONFIG ?= k62v1_64_pax_debug_defconfig
endif
ifeq ($(TARGET_BUILD_VARIANT), user)
KERNEL_DEFCONFIG ?= k62v1_64_pax_defconfig
endif
ifeq ($(TARGET_BUILD_VARIANT), userdebug)
KERNEL_DEFCONFIG ?= k62v1_64_pax_defconfig userdebug.config
endif
```

# qcm2290 bengal平台

* `device/qcom/kernelscripts/kernel_definitions.mk`这里user版本使用的是`bengal-perf_defconfig`，非user使用的是`bengal_defconfig`:
```shell
# Android Kernel compilation/common definitions
ifeq ($(TARGET_BUILD_VARIANT),user)
     KERNEL_DEFCONFIG ?= vendor/$(TARGET_BOARD_PLATFORM)-perf_defconfig
endif

ifeq ($(KERNEL_DEFCONFIG),)
     KERNEL_DEFCONFIG := vendor/$(TARGET_PRODUCT)_defconfig
endif

ifeq ($(shell echo $(KERNEL_DEFCONFIG) | grep vendor),)
KERNEL_DEFCONFIG := vendor/$(KERNEL_DEFCONFIG)
endif
```


# 控制kernel编译的AndroidKernel.mk
在内核源码中的`kernel/msm-4.14/AndroidKernel.mk`文件确定了具体使用哪个config文件:
```shell
KERNEL_HEADER_DEFCONFIG := $(strip $(KERNEL_HEADER_DEFCONFIG))
ifeq ($(KERNEL_HEADER_DEFCONFIG),)
KERNEL_HEADER_DEFCONFIG := $(KERNEL_DEFCONFIG)
endif
```