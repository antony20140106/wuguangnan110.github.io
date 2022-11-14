# 概述

Android系统源码全编译时，都会通过PRODUCT_PACKAGES来控制源码模块是否需要编译， 对于系统默认的一些模块， 如果想要进行裁剪， 比如pos上面，可能不需要电话，短信等APP， 此时就需要修改系统基础的配置文件， 将PRODUCT_PACKAGES中包含的电话短信APP模块删除， 这样改动太大， 耦合性也太大， 我们可以在Android系统源码的编译规则中增加一个`PRODUCT_REMOVE_PACKAGES`变量来对模块进行删除，实现模块裁剪的功效。当然裁剪不仅仅局限于系统APP， 源码中所有的模块都可以通过`PRODUCT_REMOVE_PACKAGES`来裁剪。

# 参考

* [Android12 系统的裁剪编译规则](https://blog.csdn.net/ldswfun/article/details/119783255)

# 软件修改历程

```diff
--- a/UM.9.15/build/make/core/main.mk
+++ b/UM.9.15/build/make/core/main.mk
@@ -1063,6 +1063,7 @@ endef
 #   If a module is built for 2nd arch, its required module resolves to
 #   32-bit variant, if it exits. See the select-bitness-of-required-modules definition.
 # $(1): product makefile
+#[FEATURE]-Add by wugangnan@paxsz.com, 2022/11/06, add PRODUCT_PACKAGES_REMOVE for delete unuse package
 define product-installed-files
   $(eval _pif_modules := \
     $(call get-product-var,$(1),PRODUCT_PACKAGES) \
@@ -1076,6 +1077,8 @@ define product-installed-files
   $(eval ### Filter out the overridden packages and executables before doing expansion) \
   $(eval _pif_overrides := $(call module-overrides,$(_pif_modules))) \
   $(eval _pif_modules := $(filter-out $(_pif_overrides), $(_pif_modules))) \
+  $(eval _pif_dels := $(call get-product-var,$(1),PRODUCT_PACKAGES_REMOVE)) \
+  $(eval _pif_modules := $(filter-out $(_pif_dels), $(_pif_modules))) \
   $(eval ### Resolve the :32 :64 module name) \
   $(eval _pif_modules_32 := $(patsubst %:32,%,$(filter %:32, $(_pif_modules)))) \
   $(eval _pif_modules_64 := $(patsubst %:64,%,$(filter %:64, $(_pif_modules)))) \
diff --git a/UM.9.15/build/make/core/product.mk b/UM.9.15/build/make/core/product.mk
index edd24ae4a74..d936d3b9d8e 100644
--- a/UM.9.15/build/make/core/product.mk
+++ b/UM.9.15/build/make/core/product.mk
@@ -117,6 +117,11 @@ _product_list_vars :=
 _product_single_value_vars += PRODUCT_NAME
 _product_single_value_vars += PRODUCT_MODEL

+#[NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-11-06,remove unused module
+_product_single_value_vars += PRODUCT_PACKAGES_REMOVE
+#[NEW FEATURE]-END by wugangnan@paxsz.com 2022-11-06,remove unused module

--- a/UM.9.15/device/qcom/bengal/bengal.mk
+++ b/UM.9.15/device/qcom/bengal/bengal.mk
@@ -265,6 +265,10 @@ PRODUCT_PACKAGES += android.hardware.lights-service.qti
 PRODUCT_PACKAGES += libqlmodem
 PRODUCT_PACKAGES += android.hardware.quectelat@1.0-service

+#[NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-11-06,remove unused module
+PRODUCT_PACKAGES_REMOVE += \
+    hvdcp_opti
+#[NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-11-06,remove unused module
```