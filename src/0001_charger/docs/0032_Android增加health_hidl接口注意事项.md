# Android增加health hidl接口注意事项

尝试增加healthd HIDL相关接口，发现还需要修改lsdump部分，研究一下。

# steps

* `development/vndk/tools/header-checker/utils/create_reference_dumps.py -l android.hardware.health@1.0 -product bengal_32go`
```log
device/qcom/bengal/AndroidBoard.mk:103: warning: ignoring old commands for target `out/target/product/bengal/recovery.img'
[100% 1688/1688] //hardware/interfaces/health/1.0:android.hardware.health@1.0 header-abi-linker android.hardware.health@1.0.so.lsdump [arm]
Creating dumps for target_arch: arm and variant  armv7-a-neon
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm_armv7-a-neon/source-based/android.hardware.health@1.0.so.lsdump
Creating dumps for target_arch: arm64 and variant  armv8-a
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm64_armv8-a/source-based/android.hardware.health@1.0.so.lsdump
```

* `evelopment/vndk/tools/header-checker/utils/create_reference_dumps.py -l android.hardware.health@2.1 -product bengal_32go`:
```log
Creating dumps for target_arch: arm and variant  armv8-a
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm_armv8-a/source-based/android.hardware.health@2.1.so.lsdump
```

* `development/vndk/tools/header-checker/utils/create_reference_dumps.py -l android.hardware.health@2.0 -product bengal_32go` 
```log
Creating dumps for target_arch: arm and variant  armv8-a
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm_armv8-a/source-based/android.hardware.health@2.0.so.lsdump
```

* `evelopment/vndk/tools/header-checker/utils/create_reference_dumps.py -l android.hardware.health@2.1 -product bengal`:
```log
Creating dumps for target_arch: arm and variant  armv7-a-neon
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm_armv7-a-neon/source-based/android.hardware.health@2.1.so.lsdump
Creating dumps for target_arch: arm64 and variant  armv8-a
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm64_armv8-a/source-based/android.hardware.health@2.1.so.lsdump
```

* `evelopment/vndk/tools/header-checker/utils/create_reference_dumps.py -l android.hardware.health@2.0 -product bengal`:
```log
Creating dumps for target_arch: arm and variant  armv7-a-neon
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm_armv7-a-neon/source-based/android.hardware.health@2.0.so.lsdump
Creating dumps for target_arch: arm64 and variant  armv8-a
Created abi dump at /home/wugn/gms_branch_m9200/A6650-project/UM.9.15/prebuilts/abi-dumps/vndk/30/64/arm64_armv8-a/source-based/android.hardware.health@2.0.so.lsdump
```

# 报错

```log

[   38.486480] ipa-wan ipa3_wwan_ioctl:1967 dev(rmnet_data0) register to IPA
[   39.761698] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   40.762905] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   41.155826] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   41.202586] msm_pcm_volume_ctl_get substream not found
[   41.725673] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   41.766736] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   41.777429] msm_pcm_volume_ctl_get substream not found
[   43.825313] ipa-wan ipa3_wwan_ioctl:1967 dev(rmnet_data10) register to IPA
[   44.849080] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   45.136467] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   45.855573] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   46.213579] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   46.261395] msm_pcm_volume_ctl_get substream not found
[   46.484582] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   46.516720] msm_pcm_volume_ctl_get substream not found
[   47.214965] [schedu][0x3cf825b3][20:45:14.789900] wlan: [1589:E:SYS] Processing SYS MC STOP
[   47.398097] [kworke][0x3d2dcc7f][20:45:14.973030] wlan: [30:E:WMI] WMI handle is NULL
[   49.854127] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   50.090187] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   50.856529] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   51.062210] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   51.092702] msm_pcm_volume_ctl_get substream not found
[   51.338562] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   51.365497] msm_pcm_volume_ctl_get substream not found
[   55.090877] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   55.864285] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   56.057892] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   56.088640] msm_pcm_volume_ctl_get substream not found
[   56.092834] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   56.692764] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   56.719504] msm_pcm_volume_ctl_get substream not found
[   60.102002] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   60.867102] init: Control message: Could not find 'android.hardware.health@2.1::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   61.052602] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
[   61.083805] msm_pcm_volume_ctl_get substream not found
[   61.104390] init: Control message: Could not find 'android.hardware.health@2.0::IHealth/default' for ctl.interface_start from pid: 401 (/system/bin/hwservicemanager)
[   61.338187] msm_qti_pp_get_rms_value_control, back not active to query rms be_idx:3
```