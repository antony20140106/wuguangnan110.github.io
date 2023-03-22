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

# 编译system还是vendor应用

比如我们想将应用放到文件系统vendor/bin目录下，这个必须是`lunch target`目录，目前qcm2290是UM9.15目录，操作如下：
* 1. 在target目录下加入包名：
```diff
--- a/UM.9.15/device/qcom/bengal/bengal.mk
+++ b/UM.9.15/device/qcom/bengal/bengal.mk
@@ -261,6 +261,7 @@ PRODUCT_PACKAGES += android.hardware.lights-service.qti
 PRODUCT_PACKAGES += libqlmodem
 PRODUCT_PACKAGES += android.hardware.quectelat@1.0-service

+PRODUCT_PACKAGES += tptool
```
* 2. 将应用放到`vendor/xxxxx/tptool`目录，Android.bp内容必须加`vendor: true`或者Android.mk中增加`LOCAL_PROPRIETARY_MODULE := true`：
```
Android.bp  tptool.cpp

Android.bp内容:
cc_binary {
    name: "tptool",
    vendor: true,

Android.mk:
LOCAL_PROPRIETARY_MODULE := true
```

system应用如下：
```diff
--- a/QSSI.12/device/qcom/qssi/qssi.mk
+++ b/QSSI.12/device/qcom/qssi/qssi.mk
@@ -299,6 +299,11 @@ ifeq (true,$(call math_gt_or_eq,$(SHIPPING_API_LEVEL),29))
   PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := true
 endif

+#[NEW FEATURE]-BEGIN by xxx@xxxxx.com 2022-10-10 for BMS notify function
+PRODUCT_PACKAGES += \
+    batterywarning
+#[NEW FEATURE]-END by xxx@xxxxx.com 2022-10-10 for BMS notify function

--- /dev/null
+++ b/QSSI.12/frameworks/opt/batterywarning/Android.bp
@@ -0,0 +1,29 @@
+cc_binary {
+    name: "batterywarning",
+    vendor: false,
+       init_rc: ["batterywarning.rc"],
+
+    cflags: [
+        "-Wno-unused-parameter",
+        "-Wno-unused-variable",
+    ],
+
+    srcs: [
+        "batterywarning.cpp",
+
+    ],
+
+    shared_libs: [
+        "libc",
+        "libcutils",
+        "libutils",
+        "libbinder",
+        "liblog",
+    ],
+
+
+    local_include_dirs: [
+        "."
+    ],
+}
+
```