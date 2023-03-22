# 概述

将高通A665x平台的kernel编译流程分析一下。

# 涉及文件

```
device/qcom/bengal/AndroidBoard.mk:
#----------------------------------------------------------------------
# Compile Linux Kernel
#----------------------------------------------------------------------
include device/qcom/kernelscripts/kernel_definitions.mk


device/qcom/kernelscripts/kernel_definitions.mk
device/qcom/kernelscripts/
├── build.config -> ../build.config
├── build.config.net_test
├── buildkernel.sh
├── build.sh
├── build_test.sh
├── envsetup.sh
├── fetch_kpb.sh
├── kernel_definitions.mk
├── LICENSE
├── METADATA
├── MODULE_LICENSE_APACHE2
├── NOTICE -> LICENSE
└── static_analysis
    ├── checkpatch_blacklist
    ├── checkpatch_presubmit.sh
    └── checkpatch.sh
```

# kernel_definitions.mk分析

* `kernel_definitions.mk`:

```makefile

```

* `device/qcom/bengal/BoardConfig.mk`:
```
#Disable appended dtb.
TARGET_KERNEL_APPEND_DTB := false
```
