# README

以高通平台qcm2290为例，分析一下lunch product到底干了什么，又是如何检索所有的mk文件的，`AndroidProducts.mk`这个是Android lunch的起始文件分析一下，有助于理解device下的架构。

# refers

* [七、AndroidProducts.mk](https://blog.csdn.net/daoshuti/article/details/108513784)

# 分析

我们A6650项目目前编译的项目名为`bengal`，首先看看其目录下`AndroidProducts.mk`内容：
```shell
wugn@jcrj-tf-compile:UM.9.15$ cat device/qcom/bengal/AndroidProducts.mk
PRODUCT_MAKEFILES := \
        $(LOCAL_DIR)/bengal.mk

COMMON_LUNCH_CHOICES := \
        bengal-userdebug \
        bengal-user \
        bengal-wifionlyuserdebug \
        bengal-wifionlyuser
```

从上面的配置可知，qcom在bengal源码中加了如下四个默认的`lunch-combo`配置，lunch-combo会用-分隔，前面的是项目名、后面是版本类型（user、userdebug、eng）。
```shell
bengal-userdebug 
bengal-user 
bengal-wifionlyuserdebug 
bengal-wifionlyuser
```

# AndroidProducts.mk如何被加载

* 当用户执行lunch命令时会调用get_build_var、build_build_var_cache等shell函数
* 这些shell函数又会调用build/soong/soong_ui.bash --dumpvar-mode
* soong_ui.bash走到/build/soong/cmd/soong_ui/main.go中的main函数中
* main函数中调用build.FindSources(buildCtx, config, f)

* FindSources代码逻辑如下:
  * 在device、vendor、product目录中查找AndroidProducts.mk文件。
  * 并将所有的名为AndroidProducts.mk文件路径记录在AndroidProducts.mk.list中
备注：这函数的作用不止如此，还会查找`Android.mk`、`Android.bp`、`CleanSpec.mk`等文件。

# 查看bengal.mk包含关系

* `device/qcom/bengal/bengal.mk`主要看`common64.mk`:
```shell
ifeq ($(ENABLE_VIRTUAL_AB), true)
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota.mk)
endif

ifneq ($(strip $(BOARD_DYNAMIC_PARTITION_ENABLE)),true)
$(call inherit-product, build/make/target/product/gsi_keys.mk)
endif

#这个重要
$(call inherit-product, device/qcom/vendor-common/common64.mk)

#下面两个没有：
###################################################################################
# This is the End of target.mk file.
# Now, Pickup other split product.mk files:
###################################################################################
# TODO: Relocate the system product.mk files pickup into qssi lunch, once it is up.
$(call inherit-product-if-exists, vendor/qcom/defs/product-defs/system/*.mk)
$(call inherit-product-if-exists, vendor/qcom/defs/product-defs/vendor/*.mk)
```

* `vendor-common/common64.mk`:
```shell
$(call inherit-product, device/qcom/vendor-common/base.mk)

# For PRODUCT_COPY_FILES, the first instance takes precedence.
# Since we want use QC specific files, we should inherit
# device-vendor.mk first to make sure QC specific files gets installed.
$(call inherit-product-if-exists, $(QCPATH)/common/config/device-vendor-qssi.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)
```

* bengal.mk
  * common64.mk
    * base.mk