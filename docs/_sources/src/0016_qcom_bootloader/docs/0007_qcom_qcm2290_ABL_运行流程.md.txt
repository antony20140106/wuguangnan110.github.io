# README

在前面[0005_qcom_XBL运行流程.md](0005_qcom_XBL运行流程.md)中，我们分析到，在BdsEntry() 中会调用 LaunchDefaultBDSApps() 回载默认app，包含ABL。


# 概述

默认app定义在 amss\BOOT.XF.1.4\boot_images\QcomPkg\Sdm660Pkg\LA\uefiplat.cfg 中，如下：
```
## Default app to boot in platform BDS init
DefaultChargerApp = "QcomChargerApp"
DefaultBDSBootApp = "LinuxLoader"
```

# 参考

* [【Android SDM660源码分析】- 04 - UEFI ABL LinuxLoader 代码分析](https://blog.csdn.net/Ciellee/article/details/114406051)

# 1. LinuxLoader.c

高通代码中 LinuxLoader 代码位于 `bootable\bootloader\edk2\QcomModulePkg\Application\LinuxLoader` 
中。

从 LinuxLoader\LinuxLoader.inf 可知其入口函数为 LinuxLoaderEntry()
其主要工作如下：

1. 分配栈内存
2. 如果是首次开机，先读取设备信息，判断是否需要做verity boot验证，然后更新devinfo区域
3. 枚举当前设备的emmc 分区
4. 判断启动模式，fastboot/RECOVERY/ALARM_BOOT/DM_VERITY_ENFORCING/DM_VERITY_LOGGING/DM_VERITY_KEYSCLEAR启动


```C++
# bootable\bootloader\edk2\QcomModulePkg\Application\LinuxLoader\LinuxLoader.c
/** Linux Loader Application EntryPoint **/

EFI_STATUS EFIAPI  __attribute__ ( (no_sanitize ("safe-stack")))
LinuxLoaderEntry (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{

  DEBUG ((EFI_D_INFO, "Loader Build Info: %a %a\n", __DATE__, __TIME__));
  DEBUG ((EFI_D_VERBOSE, "LinuxLoader Load Address to debug ABL: 0x%llx\n", (UINTN)LinuxLoaderEntry & (~ (0xFFF))));
  DEBUG ((EFI_D_VERBOSE, "LinuxLoaderEntry Address: 0x%llx\n", (UINTN)LinuxLoaderEntry));
  // 1. 分配栈内存
  Status = AllocateUnSafeStackPtr ();
  StackGuardChkSetup ();
  BootStatsSetTimeStamp (BS_BL_START);

  // 2. 如果是首次开机，先读取设备信息，判断是否需要做verity boot验证，然后更新devinfo区域。Initialize verified boot & Read Device Info
  Status = DeviceInfoInit ();
  // 3. 枚举当前设备的emmc 分区
  Status = EnumeratePartitions ();

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "LinuxLoader: Could not enumerate partitions: %r\n",
            Status));
    goto stack_guard_update_default;
  }

  UpdatePartitionEntries ();
  /*Check for multislot boot support*/
  MultiSlotBoot = PartitionHasMultiSlot ((CONST CHAR16 *)L"boot");
  if (MultiSlotBoot) {
    DEBUG ((EFI_D_VERBOSE, "Multi Slot boot is supported\n"));
    FindPtnActiveSlot ();
  }

  Status = GetKeyPress (&KeyPressed);
  if (Status == EFI_SUCCESS) {
    if (KeyPressed == SCAN_DOWN)
      BootIntoFastboot = TRUE;
    if (KeyPressed == SCAN_UP)
      BootIntoRecovery = TRUE;
    if (KeyPressed == SCAN_ESC)
      RebootDevice (EMERGENCY_DLOAD);
  } else if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((EFI_D_ERROR, "Error reading key status: %r\n", Status));
    goto stack_guard_update_default;
  }

  // check for reboot mode
  Status = GetRebootReason (&BootReason);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Failed to get Reboot reason: %r\n", Status));
    goto stack_guard_update_default;
  }

  switch (BootReason) {
  case FASTBOOT_MODE:
    BootIntoFastboot = TRUE;
    break;
  case RECOVERY_MODE:
    BootIntoRecovery = TRUE;
    break;
  case ALARM_BOOT:
    BootReasonAlarm = TRUE;
    break;
  case DM_VERITY_ENFORCING:
    // write to device info
    Status = EnableEnforcingMode (TRUE);
    if (Status != EFI_SUCCESS)
      goto stack_guard_update_default;
    break;
  case DM_VERITY_LOGGING:
    /* Disable MDTP if it's Enabled through Local Deactivation */
    Status = MdtpDisable ();
    if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
      DEBUG ((EFI_D_ERROR, "MdtpDisable Returned error: %r\n", Status));
      goto stack_guard_update_default;
    }
    // write to device info
    Status = EnableEnforcingMode (FALSE);
    if (Status != EFI_SUCCESS)
      goto stack_guard_update_default;

    break;
  case DM_VERITY_KEYSCLEAR:
    Status = ResetDeviceState ();
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "VB Reset Device State error: %r\n", Status));
      goto stack_guard_update_default;
    }
    break;
  default:
    if (BootReason != NORMAL_MODE) {
      DEBUG ((EFI_D_ERROR,
             "Boot reason: 0x%x not handled, defaulting to Normal Boot\n",
             BootReason));
    }
    break;
  }

  Status = RecoveryInit (&BootIntoRecovery);
  if (Status != EFI_SUCCESS)
    DEBUG ((EFI_D_VERBOSE, "RecoveryInit failed ignore: %r\n", Status));

  /* Populate board data required for fastboot, dtb selection and cmd line */
  Status = BoardInit ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error finding board information: %r\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "KeyPress:%u, BootReason:%u\n", KeyPressed, BootReason));
  DEBUG ((EFI_D_INFO, "Fastboot=%d, Recovery:%d\n",
                                          BootIntoFastboot, BootIntoRecovery));
  if (!GetVmData ()) {
    DEBUG ((EFI_D_ERROR, "VM Hyp calls not present\n"));
  }

  if (!BootIntoFastboot) {
    BootInfo Info = {0};
    Info.MultiSlotBoot = MultiSlotBoot;
    Info.BootIntoRecovery = BootIntoRecovery;
    Info.BootReasonAlarm = BootReasonAlarm;
    Status = LoadImageAndAuth (&Info);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "LoadImageAndAuth failed: %r\n", Status));
      goto fastboot;
    }

    BootLinux (&Info);
  }

fastboot:
  DEBUG ((EFI_D_INFO, "Launching fastboot\n"));
  Status = FastbootInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Launch Fastboot App: %d\n", Status));
    goto stack_guard_update_default;
  }

stack_guard_update_default:
  /*Update stack check guard with defualt value then return*/
  __stack_chk_guard = DEFAULT_STACK_CHK_GUARD;
  return Status;
}
```