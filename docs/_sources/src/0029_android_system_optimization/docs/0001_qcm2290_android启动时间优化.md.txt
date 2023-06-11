# README

对于Android系统启动时间进行优化。

# refers


# 问题

## 1.编译报错

```diff
--- a/UM.9.15/kernel/msm-4.19/arch/arm/configs/vendor/bengal-perf_defconfig
+++ b/UM.9.15/kernel/msm-4.19/arch/arm/configs/vendor/bengal-perf_defconfig
@@ -143,7 +143,7 @@ CONFIG_NETFILTER_XT_TARGET_CONNMARK=y
 CONFIG_NETFILTER_XT_TARGET_CONNSECMARK=y
 CONFIG_NETFILTER_XT_MATCH_QUOTA2=y
-CONFIG_NETFILTER_XT_MATCH_QUOTA2_LOG=y
+#CONFIG_NETFILTER_XT_MATCH_QUOTA2_LOG=y #[BUGFIX]-0046628-BEGIN by (wugangnan@paxsz.com), 2023/04/26 for reduce boot time
 # CONFIG_NETFILTER_XT_MATCH_SCTP is not set
```

编译报错如下，提示不能去掉`CONFIG_NETFILTER_XT_MATCH_QUOTA2_LOG`：
```log
checkvintf I 05-10 19:35:32 44855 44855 check_vintf.cpp:84] List 'out/target/product/bengal_32go/system/product/etc/vintf/': No such file or directory
checkvintf E 05-10 19:35:32 44855 44855 check_vintf.cpp:554] files are incompatible: Runtime info and framework compatibility matrix are incompatible: No compatible kernel requirement foundernel FCM version = 5).
checkvintf E 05-10 19:35:32 44855 44855 check_vintf.cpp:554] For kernel requirements at matrix level 5, Missing config CONFIG_NETFILTER_XT_MATCH_QUOTA2_LOG //这里
checkvintf E 05-10 19:35:32 44855 44855 check_vintf.cpp:554] : Success
INCOMPATIBLE
[ 77% 35012/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35013/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35014/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35015/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35016/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35017/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk clang stub.c
[ 77% 35018/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35019/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk clang stub.c
[ 77% 35020/45454] //system/core/liblog:liblog generate stubs liblog.map.txt
[ 77% 35021/45454] //frameworks/base/native/graphics/jni:libjnigraphics.ndk generate stubs libjnigraphics.map.txt
[ 77% 35022/45454] build out/target/product/bengal_32go/obj/ETC/system_ext_sepolicy.cil_intermediates/system_ext_sepolicy.cil
ninja: build stopped: subcommand failed.
19:35:35 ninja failed with: exit status 1

#### failed to build some targets (24:38 (mm:ss)) ####
```