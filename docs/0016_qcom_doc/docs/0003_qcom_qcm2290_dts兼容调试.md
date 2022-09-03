# 概述

将高通A6650平台的dts整合在一起。

# 参考

* [shell实用脚本：批量拷贝相同类型的文件](https://blog.csdn.net/linux_player_c/article/details/50965855)

# 调试流程

## 拷贝dtb文件

* 参考1400代码，在dtb根目录下修改Makefile如下：

```
ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dts-dirs += a6650
dts-dirs += m9200
endif
```

* 目录结构：

```
wugn@jcrj-tf-compile:qcom$ tree a6650/
a6650/
├── a6650-scuba-iot.dts
├── a6650-scuba-iot-idp-overlay.dts
├── Makefile
```

* a6650目录下Makefile如下：

```
ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_SCUBA) += a6650-scuba-iot-idp-overlay.dtbo
a6650-scuba-iot-idp-overlay.dtbo-base := a6650-scuba-iot.dtb
endif

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)

clean-files	:= *.dtb *.dtbo
```

* 根据dts文件中`#include "scuba-iot.dtsi"`和`#include "scuba-iot-idp.dtsi"`将这几个文件加进来，加进来后直接搜索include关键字如下：

```c
scuba-audio.dtsi
1:#include <dt-bindings/clock/qcom,audio-ext-clk.h>
2:#include "msm-audio-lpass.dtsi"
22:#include "scuba-lpi.dtsi"

scuba.dtsi
2394:#include "pm2250.dtsi"
2395:#include "scuba-thermal.dtsi"
2396:#include "scuba-coresight.dtsi"
2397:#include "scuba-pinctrl.dtsi"
2398:#include "scuba-ion.dtsi"
2399:#include "pm2250-rpm-regulator.dtsi"
2400:#include "scuba-regulator.dtsi"
2401:#include "scuba-gdsc.dtsi"
2402:#include "scuba-qupv3.dtsi"
2403:#include "scuba-audio.dtsi"
2404:#include "scuba-usb.dtsi"
2405:#include "msm-arm-smmu-scuba.dtsi"
2406:#include "scuba-bus.dtsi"
2407:#include "scuba-gpu.dtsi"
2408:#include "scuba-vidc.dtsi"
2412:   #include "pm8008.dtsi"
2684:#include "scuba-pm.dtsi"
2685:#include "scuba-sde.dtsi"
2686:#include "scuba-sde-pll.dtsi"
2687:#include "camera/scuba-camera.dtsi"
2694:#include "msm-rdbg-scuba.dtsi"

scuba-sde-display.dtsi
2:#include "dsi-panel-nt36525-truly-hd-plus-vid.dtsi"
3:#include "dsi-panel-td4330-truly-v2-singlemipi-fhd-vid.dtsi"
4:#include "dsi-panel-td4330-truly-v2-singlemipi-fhd-cmd.dtsi"
5:#include "dsi-panel-hx8394f-720p-video.dtsi"
6:#include "dsi-panel-ili9881d-720p-video.dtsi"

1:#include "scuba-audio-overlay.dtsi"
5:#include "scuba-thermal-overlay.dtsi"
6:#include "scuba-sde-display.dtsi"
7:#include "camera/scuba-camera-sensor-idp.dtsi"
12:#include "qg-batterydata-alium-3600mah.dtsi"
13:#include "qg-batterydata-atl466271_3300mAh.dtsi"

scuba-iot.dtsi
1:#include "scuba.dtsi"
```

将以上关联的dtsi文件全部加进来，目前目录如下：

```c
wugn@jcrj-tf-compile:a6650$ tree
.
├── a6650-scuba-iot.dts
├── a6650-scuba-iot-idp-overlay.dts
├── camera
│   ├── scuba-camera.dtsi
│   └── scuba-camera-sensor-idp.dtsi
├── dsi-panel-hx8394f-720p-video.dtsi
├── dsi-panel-ili9881d-720p-video.dtsi
├── dsi-panel-nt36525-truly-hd-plus-vid.dtsi
├── dsi-panel-td4330-truly-v2-singlemipi-fhd-cmd.dtsi
├── dsi-panel-td4330-truly-v2-singlemipi-fhd-vid.dtsi
├── Makefile
├── msm-arm-smmu-scuba.dtsi
├── msm-audio-lpass.dtsi
├── msm-rdbg-scuba.dtsi
├── pm2250.dtsi
├── pm2250-rpm-regulator.dtsi
├── pm8008.dtsi
├── qg-batterydata-alium-3600mah.dtsi
├── qg-batterydata-atl466271_3300mAh.dtsi
├── scuba-audio.dtsi
├── scuba-audio-overlay.dtsi
├── scuba-bus.dtsi
├── scuba-coresight.dtsi
├── scuba.dtsi
├── scuba-gdsc.dtsi
├── scuba-gpu.dtsi
├── scuba-ion.dtsi
├── scuba-iot.dtsi
├── scuba-iot-idp.dtsi
├── scuba-lpi.dtsi
├── scuba-pinctrl.dtsi
├── scuba-pm.dtsi
├── scuba-qupv3.dtsi
├── scuba-regulator.dtsi
├── scuba-rumi.dtsi
├── scuba-sde-display.dtsi
├── scuba-sde.dtsi
├── scuba-sde-pll.dtsi
├── scuba-thermal.dtsi
├── scuba-thermal-overlay.dtsi
├── scuba-usb.dtsi
└── scuba-vidc.dtsi
```

## 报错分析

编译报错如下：

```log
Copying kernel image to prebuilt
=============
Copying target dtb/dtbo files to prebuilt
cp: bad '/home/wugn/A6650-project/UM.9.15/out/target/product/bengal/obj/kernel/msm-4.19/arch/arm64/boot/dts/vendor/qcom/*.dtb': No such file or directory
[ 93% 2501/2665] target SharedLib: libril-qc-hal-qmi (out/target/product/bengal/obj/SHARED_LIBRARIES/libril-qc-hal-qmi_intermediates/LINKED/libril-qc-hal-qmi.so)
[ 93% 2502/2665] target SharedLib: camera.qcom (out/target/product/bengal/obj/SHARED_LIBRARIES/camera.qcom_intermediates/LINKED/camera.qcom.so)
ninja: build stopped: subcommand failed.
09:19:47 ninja failed with: exit status 1
```

以上可知dtb生成目录为`out/target/product/bengal/obj/kernel/msm-4.19/arch/arm64/boot/dts/vendor/qcom/`:
```
wugn@jcrj-tf-compile:qcom$ tree
.
├── a6650
│   ├── a6650-scuba-iot.dtb
│   ├── a6650-scuba-iot-idp-overlay.dtbo
│   └── modules.order
├── m9200
│   ├── m9200-scuba-iot.dtb
│   ├── m9200-scuba-iot-idp-overlay.dtbo
│   └── modules.order
└── modules.order
```

定位到脚本，原因是生成的dtb文件都放进a6650文件夹里面，找不到路径`device/qcom/kernelscripts/buildkernel.sh +185`修改如下，将`a6650`和`m9200`两个文件夹下的`dtb`和`dtbo`都拷贝过去：

```shell
PAX_DTBO_FILE="
a6650
m9200
"

copy_all_to_prebuilt()
{
    for FILE in $PAX_DTBO_FILE
    do
            #cp -p -r ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/*.dtb ${PREBUILT_OUT}/${IMAGE_FILE_PATH}/dts/vendor/qcom/
            #if [[ -e ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/*.dtbo ]]; then
            cp -p -r ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/$FILE/*.dtb ${PREBUILT_OUT}/${IMAGE_FILE_PATH}/dts/vendor/qcom
            if [[ -e ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/$FILE/*.dtbo ]]; then
                    cp -p -r ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/$FILE/*.dtbo ${PREBUILT_OUT}/${IMAGE_FILE_PATH}/dts/vendor/qcom/
            fi
            echo "${OUT_DIR}/$PAX_DTBO_FILE copied to ${PREBUILT_OUT}"
    done
}
```

## 继续报错

可以看到拷贝动作是没问题了，哪个地方cat之前错误的路径有问题。

```log
Copying kernel image to prebuilt
=============
Copying target dtb/dtbo files to prebuilt
/home/wugn/A6650-project/UM.9.15/out/target/product/bengal/obj/kernel/msm-4.19/
a6650
m9200
 copied to /home/wugn/A6650-project/UM.9.15/kernel/ship_prebuilt/primary_kernel
/home/wugn/A6650-project/UM.9.15/out/target/product/bengal/obj/kernel/msm-4.19/
a6650
m9200
 copied to /home/wugn/A6650-project/UM.9.15/kernel/ship_prebuilt/primary_kernel
=============
Copying arch-specific generated headers to prebuilt
=============
Copying kernel generated headers to prebuilt
============
Copying userspace headers to prebuilt
============
Copying kernel scripts to prebuilt
[ 93% 2501/2665] Prebuilt:  (out/target/product/bengal/kernel)
[ 93% 2502/2665] build out/target/product/bengal/obj/KERNEL_OBJ/rtic_mp.dtb
[ 93% 2503/2665] Target dtbo image: out/target/product/bengal/prebuilt_dtbo.img
create image file: out/target/product/bengal/prebuilt_dtbo.img...
Total 2 entries.
[ 93% 2504/2665] build out/target/product/bengal/dtb.img
FAILED: out/target/product/bengal/dtb.img
/bin/bash -c "cat out/target/product/bengal/obj/kernel/msm-4.19/arch/arm64/boot/dts/vendor/qcom/*.dtb out/target/product/bengal/obj/KERNEL_OBJ/rtic_mp.dtb > out/target/product/bengal/dtb.img"
cat: out/target/product/bengal/obj/kernel/msm-4.19/arch/arm64/boot/dts/vendor/qcom/*.dtb: No such file or directory
```

解决方案：将dtb和dtbo文件直接挪出来。

```shell
copy_all_to_prebuilt()
{
	#[NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-07-08, for multi dtb
	PAX_DTB_FILE=`find ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom -name "*.dtb"`
	PAX_DTBO_FILE=`find ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom -name "*.dtbo"`

	for FILE in $PAX_DTB_FILE
	do
		mv ${FILE} ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/
		echo "${FILE} move to ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/"
	done
	for FILE in $PAX_DTBO_FILE
	do
		if [[ -e ${FILE} ]]; then
			mv ${FILE} ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/
		fi
		echo "${FILE} move to ${OUT_DIR}/${IMAGE_FILE_PATH}/dts/vendor/qcom/"
	done
	#[NEW FEATURE]-END by wugangnan@paxsz.com 2022-07-08, for multi dtb
}
```