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

# fastboot最小电压要求

* `QcomPkg/SocPkg/AgattiPkg/Settings/Charger/QcomChargerConfig_VbattTh.cfg`定义如下,MTK平台是3.5v：
```C++
#Minimum Battery Voltage to allow SW Flash Image
SWFlashMinBattVoltageMv = 3600
```

* `QcomPkg/Drivers/QcomChargerDxe/QcomChargerPlatform.c`驱动中定义最小电压为3.6v:
```C++
/**
ChargerPlatform_ReadCfgParams()

@brief
Battery Parameter Default Configurations file read Call Back
*/
VOID
ChargerPlatform_ReadCfgParams
(
  UINT8* Section,
  UINT8* Key,
  UINT8* Value
)
{
    if (AsciiStriCmp ((CHAR8*)Key, "SWFlashMinBattVoltageMv") == 0)
    {
      gChargerPlatformCfgData.SWFlashMinBattVoltageMv = ChargerPlatformFile_AsciiToInt((char *)Value);
      return;
    }

}

EFI_STATUS ChargerPlatformFile_GetChargerConfig(EFI_QCOM_CHARGER_CONFIG_KEY ChargerCfgKey, UINT32 *KeyValue)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if(FALSE == gChargerCfgInitialized)
  {
    return EFI_DEVICE_ERROR;
  }

  if(NULL == KeyValue)
  {
    return EFI_INVALID_PARAMETER;
  }

  switch(ChargerCfgKey)
  {
    case EFI_QCOM_CHARGER_CONFIG_KEY_SW_FLASH_VOLTAGE:
      *KeyValue = gChargerPlatformCfgData.SWFlashMinBattVoltageMv;
    break;
    default:
      Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}
```

* `QcomPkg/Drivers/ChargerExDxe/ChargerExProtocol.c`ABL通过protocol的`ChargerExIsPowerOk`获取，其中调用驱动的protocol接口`GetChargerConfig`接口获取方式，主要是判断电压大于3.6v则返回TRUE：
```C++
/**
EFI_ChargerExIsPowerOk ()

@brief
Returns if battery voltage is good to process with SW flash
*/
EFI_STATUS
EFIAPI
EFI_ChargerExIsPowerOk
(
  IN  EFI_CHARGER_EX_POWER_TYPE   PowerType,
  OUT VOID                       *pPowerTypeInfo
)
{
  EFI_STATUS Status                = EFI_SUCCESS;
  EFI_CHARGER_EX_FLASH_INFO *pFlashInfo = NULL;
  UINT32     SwFlashBattMinVoltage = 0;
  UINT32     BatteryCurrentVoltage = 0;
  BOOLEAN    BatteryPresent        = FALSE;
  BOOLEAN    ChargerPresent        = FALSE;
  EFI_PLATFORMINFO_PLATFORM_TYPE       PlatformType;

  if(FALSE == IsChgPresent)
  {
        return EFI_UNSUPPORTED;
  }

  if(!pPowerTypeInfo)
    return EFI_INVALID_PARAMETER;

  pFlashInfo = (EFI_CHARGER_EX_FLASH_INFO *)pPowerTypeInfo;

  Status |= GetPlatformType(&PlatformType);
  if(EFI_SUCCESS != Status)
  {
    DEBUG((EFI_D_WARN, "ChargerExProtocol:: %a Error getting platform type  \r\n", __FUNCTION__));
    return EFI_DEVICE_ERROR;
  }

  if((EFI_PLATFORMINFO_TYPE_CDP == PlatformType) || (EFI_PLATFORMINFO_TYPE_RUMI == PlatformType))
  {
    DEBUG(( EFI_D_WARN, "ChargerExProtocol:: %a CDP/RUMI/IDP (%d) Platform detected. No Battery information available. \r\n", __FUNCTION__, PlatformType));
    return EFI_UNSUPPORTED;
  }
  else if(EFI_PLATFORMINFO_TYPE_CLS == PlatformType)
  {
    pFlashInfo->bCanFlash = TRUE;
    return EFI_SUCCESS;
  }

  switch(PowerType)
  {
    case EFI_CHARGER_EX_POWER_FLASH_BATTERY_VOLTAGE_TYPE:
      pFlashInfo = (EFI_CHARGER_EX_FLASH_INFO *)pPowerTypeInfo;
      /* Get Battery Presence, Charger Presence, Voltage */
      Status =  EFI_ChargerExGetBatteryPresence(&BatteryPresent);
      Status |= EFI_ChargerExGetChargerPresence(&ChargerPresent);
      Status |= EFI_ChargerExGetBatteryVoltage(&BatteryCurrentVoltage);

      if(EFI_SUCCESS == Status)
      {
        pFlashInfo->BattCurrVoltage = BatteryCurrentVoltage;
      }

      if(!pQcomChargerProtocol)
      {
        Status |= gBS->LocateProtocol( &gQcomChargerProtocolGuid, NULL, (VOID **)&pQcomChargerProtocol );
        if(EFI_SUCCESS != Status || NULL == pQcomChargerProtocol)
        {
          return EFI_DEVICE_ERROR;
        }
      }

      Status |= pQcomChargerProtocol->GetChargerConfig(EFI_QCOM_CHARGER_CONFIG_KEY_SW_FLASH_VOLTAGE, &SwFlashBattMinVoltage);

      if(Status == EFI_SUCCESS)
      {
        pFlashInfo->BattRequiredVoltage = SwFlashBattMinVoltage;
        /* If battery not present but still device boot up have debug board */
        if(!BatteryPresent || (BatteryPresent  && (BatteryCurrentVoltage >= SwFlashBattMinVoltage)))
        {
          pFlashInfo->bCanFlash = TRUE;
        }
        else
        {
          DEBUG(( EFI_D_WARN, "ChargerExProtocol:: %a SwFlashBattMinVoltage = %d mV\r\n", __FUNCTION__,SwFlashBattMinVoltage));
          pFlashInfo->bCanFlash = FALSE;
        }
      }
    break;
    default:
    break;
  }

  return Status;
}

/**
Charger External UEFI Protocol implementation
*/
EFI_CHARGER_EX_PROTOCOL ChargerExProtocolImplementation =
{
    CHARGER_EX_REVISION,
    EFI_ChargerExGetChargerPresence,
    EFI_ChargerExGetBatteryPresence,
    EFI_ChargerExGetBatteryVoltage,
    EFI_ChargerExIsOffModeCharging,
    EFI_ChargerExIsPowerOk,
};
```

* ABL阶段代码，如果小于3.6v则不进行充电：
```c++
/**
   Add safeguards such as refusing to flash if the battery levels is lower than
 the min voltage
   or bypass if the battery is not present.
   @param[out] BatteryVoltage  The current voltage of battery
   @retval     BOOLEAN         The value whether the device is allowed to flash
 image.
 **/
BOOLEAN
TargetBatterySocOk (UINT32 *BatteryVoltage)
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_CHARGER_EX_PROTOCOL *ChgDetectProtocol = NULL;
  EFI_CHARGER_EX_FLASH_INFO FlashInfo = {0};
  BOOLEAN BatteryPresent = FALSE;
  BOOLEAN ChargerPresent = FALSE;

  *BatteryVoltage = 0;
  Status = gBS->LocateProtocol (&gChargerExProtocolGuid, NULL,
                                (VOID **)&ChgDetectProtocol);
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((EFI_D_VERBOSE, "Charger Protocol is not available.\n"));
    return TRUE;
  } else if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error locating charger detect protocol\n"));
    return FALSE;
  }

  /* The new protocol are supported on future chipsets */
  if (ChgDetectProtocol->Revision >= CHARGER_EX_REVISION) {
    Status = ChgDetectProtocol->IsPowerOk (
        EFI_CHARGER_EX_POWER_FLASH_BATTERY_VOLTAGE_TYPE, &FlashInfo);
    if (EFI_ERROR (Status)) {
      /* But be bypassable where the device doesn't even have a battery */
      if (Status == EFI_UNSUPPORTED)
        return TRUE;

      DEBUG ((EFI_D_ERROR, "Error getting the info of charger: %r\n", Status));
      return FALSE;
    }

    *BatteryVoltage = FlashInfo.BattCurrVoltage;
    if (!(FlashInfo.bCanFlash) ||
        (*BatteryVoltage < FlashInfo.BattRequiredVoltage))
    {
      DEBUG ((EFI_D_ERROR, "Error battery voltage: %d "
        "Requireed voltage: %d, can flash: %d\n", *BatteryVoltage,
        FlashInfo.BattRequiredVoltage, FlashInfo.bCanFlash));
      return FALSE;
    }
    return TRUE;
  } else {
    Status = TargetCheckBatteryStatus (&BatteryPresent, &ChargerPresent,
                                       BatteryVoltage);
    if (((Status == EFI_SUCCESS) &&
         (!BatteryPresent ||
          (BatteryPresent && (*BatteryVoltage > BATT_MIN_VOLT)))) ||
        (Status == EFI_UNSUPPORTED)) {
      return TRUE;
    }

    DEBUG ((EFI_D_ERROR, "Error battery check status: %r voltage: %d\n",
        Status, *BatteryVoltage));
    return FALSE;
  }
}
```