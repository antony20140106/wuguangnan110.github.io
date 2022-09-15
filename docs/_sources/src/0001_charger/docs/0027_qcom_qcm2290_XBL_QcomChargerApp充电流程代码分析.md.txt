# 概述

高通平台如果达不到开机状态，也就是电池电压过低，会在XBL阶段运行comChargerApp进行充电，直到电池电压大于开机阈值则进入kernel，类似mtk平台lk阶段充电。

# 参考

* [【Android SDM660源码分析】- 02 - UEFI XBL QcomChargerApp充电流程代码分析](https://ciellee.blog.csdn.net/article/details/113759442)

# 流程图

# 加载 UEFI 默认应用程序

在高通代码中，QcomChargerApp是作为默认应用程序配置在uefiplat.cfg中`QcomPkg/SocPkg/AgattiPkg/LAA/uefiplat.cfg`：
```C++
## Default app to boot in platform BDS init
DefaultChargerApp = "QcomChargerApp"
DefaultBDSBootApp = "LinuxLoader"
```

* 系统开机过程中，初始化 Bsd 时，会调用LaunchDefaultBDSApps()函数，在该函数中，会实现对上述两个默认app 的调用。
  * 解析 uefiplat.cfg 中的 DefaultChargerApp，字符串保存在 DefaultApp 数组中。
  * 加载 QcomCharger App 应用程序，传参gMainFvGuid 和 “DefaultChargerApp”
  * 解析 uefiplat.cfg 中的 DefaultBDSBootApp，字符串保存在 DefaultApp 数组中
  * 加载 DefaultBDSBootApp 应用程序，加载后不再返回


```C++
/**
 * Launch default BDS Boot App.
 * If default BDS Boot App is specified then this function should NOT return,
 * on failure to launch or if the launched app returns, launch the shell app
 * if the device is NOT in retail mode as determined by PRODMODE Build and
 * fuse blown status.
*/
STATIC
EFI_STATUS
EFIAPI
LaunchDefaultBDSApps (VOID)
{

  EFI_STATUS Status = EFI_SUCCESS;
  CHAR8 DefaultApp[DEF_APP_STR_LEN];
  UINTN Size = DEF_APP_STR_LEN;
  CHAR8 FileinFV[64] = {0};

  //1. 解析 uefiplat.cfg中的DefaultChargerApp，字符串保存在DefaultApp数组中。
  Status = GetConfigString ("DefaultChargerApp", DefaultApp, &Size);
  AsciiStrCpy (FileinFV, FILE_IN_FV_PREPEND);
  AsciiStrCat (FileinFV, DefaultApp);

  if (Status == EFI_SUCCESS)
  {
    // 2. 加载QcomChargerApp 应用程序，传参gMainFvGuid 和 “DefaultChargerApp”
    Status = LoadImageFromFV (FileinFV, NULL );
    if (EFI_ERROR(Status))
      DEBUG((EFI_D_ERROR, "Failed to launch default charger app, status: %r\n", Status));
  }

  Size = DEF_APP_STR_LEN;

  //解析 uefiplat.cfg中的DefaultBDSBootApp，字符串保存在DefaultApp数组中。就是ABL
  Status = GetConfigString ("DefaultBDSBootApp", DefaultApp, &Size);
  if (Status == EFI_SUCCESS)
  {
    DisplayPOSTTime ();
    // 4. 加载ABL应用程序，加载后不再返回
    LaunchAppFromGuidedFv(&gEfiAblFvNameGuid, DefaultApp, NULL);
    //If we return from above function, considered a failure
    ConfirmShutdownOnFailure();
  }
  else
  {
    DEBUG ((EFI_D_INFO, "[QcomBds] No default boot app specified\n"));
  }

  return Status;
}
```

# QcomChargerApp应用程序初始化

# 入口函数 QcomChargerApp_Entry()

```shell
// amss\BOOT.XF.1.4\boot_images\QcomPkg\Application\QcomChargerApp\QcomChargerApp.inf
[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = QcomChargerApp
  FILE_GUID                      = EEE9C2B1-16CA-4f34-87EA-2E6D1E160CC4
  MODULE_TYPE                    = UEFI_APPLICATION
  VERSION_STRING                 = 1.0
  ENTRY_POINT                    = QcomChargerApp_Entry
```

* 从QcomChargerApp.inf中可知，其入口函数为 QcomChargerApp_Entry()，我们跳转进入分析下:
  * 获得当前的平台类型，如EFI_PLATFORMINFO_TYPE_MTP=0x08等
  * 如果当前平台是 EFI_PLATFORMINFO_TYPE_CDP 或 EFI_PLATFORMINFO_TYPE_RUMI，则不需要运行充电应用程序
  * 获得充电驱动的结构体函数，结构体指针保存在pQcomChargerProtocol中。
  * 获得当前的电池状态，如果不等于EFI_QCOM_CHARGER_ACTION_DEBUG_BOARD_GOOD_TO_BOOT则运行该app进行循环充电，否则退出。
  * 判断是否需要加载充电电池曲线数参数FG battery profile。
  * 如果当前电池状态不允许正常开机，则调用QcomChargerApp_Initialize() 初始化PMIC充电，且在QcomChargerApp_MonitorCharging()中开始循环监控充电状态。
  * 退出充电模式。
* `boot_images\QcomPkg\Application\QcomChargerApp\QcomChargerApp.c`:
```C++
# amss\BOOT.XF.1.4\boot_images\QcomPkg\Application\QcomChargerApp\QcomChargerApp.c

EFI_STATUS QcomChargerApp_Entry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  // 1. 获得当前的平台类型，如EFI_PLATFORMINFO_TYPE_MTP=0x08等
  Status |= GetPlatformType(&PlatformType);
  // 2. 如果当前平台是 EFI_PLATFORMINFO_TYPE_CDP  或 EFI_PLATFORMINFO_TYPE_RUMI，则不需要运行充电应用程序
  if((EFI_PLATFORMINFO_TYPE_CDP == PlatformType) || (EFI_PLATFORMINFO_TYPE_RUMI == PlatformType))
  {	/*if platform is CDP/RUMI , don't need to run the app. */
    CHARGERAPP_FILE_UART_DEBUG(( EFI_D_WARN, "QcomChargerApp:: %a CDP/RUMI (%d) Platform detected. Exiting app. \r\n", __FUNCTION__, PlatformType));
    return EFI_SUCCESS;
  }
  // 3. 获得充电驱动的结构体函数，结构体指针保存在pQcomChargerProtocol中。
  Status = gBS->LocateProtocol( &gQcomChargerProtocolGuid, NULL, (VOID **)&pQcomChargerProtocol );
  // 4. 获得当前的电池状态，如果不等于EFI_QCOM_CHARGER_ACTION_DEBUG_BOARD_GOOD_TO_BOOT
  /*Get charging Action to be taken */
  Status = pQcomChargerProtocol->GetChargingAction(&ChargingAction, &ChargerActionInfo);
  // 5. 判断是否需要加载充电电池曲线数参数FG battery profile
  /* Handle if FG battery profile needs to be loaded */
  if(EFI_QCOM_CHARGER_PLATFORM_ACTION_PROFILE_LOAD == ChargingAction){
    if(ChargerActionInfo.ProfState == EFI_QCOM_CHARGER_PROFILE_LOAD){
      Status |= QcomChargerApp_DisplaySignOfLifeOnVBat(&ChargerActionInfo);
    }
    /* Load Battery Profile */
    Status |= pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
    /*Get charging Action again to be taken to proceed further for charging if needed */
    Status = pQcomChargerProtocol->GetChargingAction(&ChargingAction, &ChargerActionInfo);
  }
  // 6. 如果当前电池状态不允许正常开机，则调用QcomChargerApp_Initialize() 显示充电图标及启动一个5s的定时器，且在QcomChargerApp_MonitorCharging()中开始循环监控充电状态
  /* Take Action First skip if debug board i.e. boot to HLOS */
  if(EFI_QCOM_CHARGER_ACTION_DEBUG_BOARD_GOOD_TO_BOOT != ChargingAction)
  {
	  Status |= pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
	  /* DO charger App Actions */
	  switch(ChargingAction)
	  {
	    case EFI_QCOM_CHARGER_ACTION_START_CHARGING:
	    case EFI_QCOM_CHARGER_ACTION_NO_CHARGE_WAIT:
	    case EFI_QCOM_CHARGER_ACTION_CONTINUE:
	      /*Initializes and displays the battery symbol*/
	      Status |= QcomChargerApp_Initialize(ChargingAction);
	      /* if initialization success then start charging upto target SoC*/
	      Status |= QcomChargerApp_MonitorCharging();
	      Status |= QcomChargerApp_PostProcessing();
	    break;
	  }
  }
  // 7. 退出充电模式
  /* After post processing call to exit module */
  Status |= QcomChargerApp_DeInitialize();
  CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Exiting \r\n", __FUNCTION__));
  return Status;
}
```

* `QcomPkg/Drivers/QcomChargerDxe/QcomCharger.c`QcomCharger驱动提供以下protocol供APP使用:
```C++
/**
  PMIC FG UEFI Protocol implementation
 */
EFI_QCOM_CHARGER_PROTOCOL QcomChargerProtocolImplementation =
{
  QCOM_CHARGER_REVISION,
  EFI_QcomChargerEnableCharging,
  EFI_QcomChargerGetMaxUsbCurrent,
  EFI_QcomChargerSetMaxUsbCurrent,
  EFI_QcomChargerGetChargingAction,
  EFI_QcomChargerTakeAction,
  EFI_QcomChargerDisplayImage,
  EFI_QcomChargerGetBatteryPresence,
  EFI_QcomChargerGetBatteryVoltage,
  EFI_QcomChargerDeInitialize,
  EFI_QcomChargerGetFileLogInfo,
  EFI_QcomChargerDumpPeripheral,
  EFI_QcomChargerIsDcInValid,
  EFI_QcomChargerGetChargerConfig,
  EFI_QcomChargerIsChargingSupported,
  EFI_QcomChargerGetDisplayImageType
};
```

#  充电初始化 QcomChargerApp_Initialize()

* 在 QcomChargerApp_Initialize() 中主要动作就是:
  * 根据获取到的电池状态显示充电图片，有
  * 创建一个定时事件，定时时间为5s，如果时间到达则会发送EVT_NOTIFY_SIGNAL事件
```C++
EFI_STATUS QcomChargerApp_Initialize( EFI_QCOM_CHARGER_ACTION_TYPE ChargingAction )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (EFI_PLATFORMINFO_TYPE_UNKNOWN == PlatformType )
  {
    Status = GetPlatformType(&PlatformType);
    if(EFI_SUCCESS != Status)
    {
      DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Error getting platform type = %r \r\n", __FUNCTION__, Status));
      return EFI_DEVICE_ERROR;
    }
  }
  if (EFI_PLATFORMINFO_TYPE_CLS != PlatformType )
  {
    if (ChargingAction == EFI_QCOM_CHARGER_ACTION_TSENSE_HIGH_WAIT)
    {
      QcomChargerAppEvent_DispBattSymbol(EFI_QCOM_CHARGER_DISP_IMAGE_TSENS_THERMAL_SYMBOL);
    }
    else if (ChargingAction == EFI_QCOM_CHARGER_ACTION_DEBUG_BOARD_WAIT)
    {
      QcomChargerAppEvent_DispBattSymbol(EFI_QCOM_CHARGER_DISP_IMAGE_DEBUG_LOW_SYMBOL);
    }
    else
    {
      QcomChargerAppEvent_DispBattSymbol(EFI_QCOM_CHARGER_DISP_IMAGE_LOWBATTERY);
    }
    /* Start required timers */
    Status |= QcomChargerAppEvent_DisplayTimerEvent(TRUE, TRUE);
  }
  else
  {
    CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Entering LPM \r\n", __FUNCTION__));  
    QcomChargerAppEvent_EnterLPM();/* Enter LPM */
  }
  CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Status = %r \r\n", __FUNCTION__, Status));

  return Status;
}

EFI_STATUS QcomChargerAppEvent_DisplayTimerEvent(BOOLEAN bDispOffTimer, BOOLEAN AnimTimer)
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_PLATFORMINFO_PLATFORM_TYPE PlatformType;
  
  Status = GetPlatformType(&PlatformType);
  if(EFI_SUCCESS != Status)
  {
	DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Error getting platform type = %r \r\n", __FUNCTION__, Status));
	return EFI_DEVICE_ERROR;
  }

  if (EFI_PLATFORMINFO_TYPE_CLS != PlatformType )
  {

  switch (bDispOffTimer)
  {
    case TRUE:
      /* Create Animation timer */
      if(NULL == EventAnimImg )
      {
        /* To Clear screen from snapdragon logo */
        if(NULL == pQcomChargerProtocol)
        {
          Status = gBS->LocateProtocol( &gQcomChargerProtocolGuid, NULL, (VOID **)&pQcomChargerProtocol );
        }
        if((Status != EFI_SUCCESS) || (NULL == pQcomChargerProtocol))
        {
          CHARGERAPP_DEBUG((EFI_D_ERROR, "QcomChargerApp:: %a locate protocol error = %r \r\n", __FUNCTION__, Status));
          return Status;
        }
        // 创建一个定时事件，定时时间为5s，如果时间到达则会发送EVT_NOTIFY_SIGNAL事件
        Status |= gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, QcomChargerAppEvent_AnimImgTimer, NULL, /*GUID Display guys provide*/ &EventAnimImg);

      }
      if (TRUE == AnimTimer)
      {
        Status |= gBS->SetTimer(EventAnimImg, TimerPeriodic, CHGAPP_ANIM_TIMER_DURATION_IN_US);
        DisplayOffCount = 0; /* Reset DisplyOff Count */
      }
      break;
    case FALSE:
      if (NULL != EventAnimImg)
      {
        Status |= gBS->CloseEvent (EventAnimImg);
        EventAnimImg = (EFI_EVENT)NULL;
      }
      break;
    default:
      break;
  }
  	}
  return Status;
}

//定时器超时函数：
//Timer event for 500ms timer to animate Image during charging while display is ON
VOID EFIAPI QcomChargerAppEvent_AnimImgTimer
(
 IN EFI_EVENT        Event,
 IN VOID             *Context
 )
{
  STATIC CHGAPP_DISP_ANIM_IMG_NUM AnimImgNum = CHGAPP_DISP_ANIM_IMG_LOW_BATTERY;
  EFI_PLATFORMINFO_PLATFORM_TYPE PlatformType;
  
  GetPlatformType(&PlatformType);

  if (EFI_PLATFORMINFO_TYPE_CLS != PlatformType )
  {
  /*periodic timer for image display every 500ms */
  if(QCOMCHARGERAPP_EVENT_ANIMATION__CHARGING_ANIM == gAnimationType)
  {
    switch(AnimImgNum)
    {
      case CHGAPP_DISP_ANIM_IMG_LOW_BATTERY:
           QcomChargerAppDisplay_DispBattSymbol(EFI_QCOM_CHARGER_DISP_IMAGE_LOWBATTERYCHARGING, FALSE);
           AnimImgNum = CHGAPP_DISP_ANIM_IMG_LOW_BATT_CHARGING;
           break;
      case CHGAPP_DISP_ANIM_IMG_LOW_BATT_CHARGING:
           QcomChargerAppDisplay_DispBattSymbol(EFI_QCOM_CHARGER_DISP_IMAGE_LOWBATTERY, FALSE);
           AnimImgNum = CHGAPP_DISP_ANIM_IMG_LOW_BATTERY;
           break;
      default:
           break;
    }
  }

  if(++DisplayOffCount >= CHGAPP_FLASH_IMAGE_COUNT)
  {
    DisplayOffCount = 0;
    QcomChargerAppEvent_DisplayOff();
  }
}
}
```

# 循环监控充电状态 QcomChargerApp_MonitorCharging()

主要是循环监控充电状态：
1. 关闭 看门狗
2. 查询当前的充电状态
3. 配置超时时间，如果配置了充电LED灯，超时时间为500ms，否则为3s
4. 循环获取当前的充电状态
5. 根据当前状态判断是否要开机、关机、继续充电
6. 开启10分钟的看门狗定时器
```C++
# amss\BOOT.XF.1.4\boot_images\QcomPkg\Application\QcomChargerApp\QcomChargerApp.c
/*
ChgAppMonitorCharging(): This is the charging loop that will keep the chargerApp running, it will also call ChargerPlatform_Periodic function to get the error status,
get charging/gauging status, control charging and determine when to exit charging loop.
*/
EFI_STATUS QcomChargerApp_MonitorCharging( VOID )
{
  CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a \r\n", __FUNCTION__));

  if(!pQcomChargerProtocol)
    Status = gBS->LocateProtocol( &gQcomChargerProtocolGuid, NULL, (VOID **)&pQcomChargerProtocol );
  // 1. 关闭 看门狗
  /* Disable crash dump watchdog only when charger app starts charging */
  gBS->SetWatchdogTimer(0, 0, 0, NULL);
  // 2. 查询当前的充电状态
  /* Query charging action */
  Status = pQcomChargerProtocol->GetChargingAction(&ChargingAction, &ChargerActionInfo);
  CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a ChargingAction = %d Status = %r LedConfigType = %d \r\n", __FUNCTION__, ChargingAction, Status, ChargerActionInfo.LedConfigType));

  CHARGERAPP_FILE_DEBUG((EFI_D_WARN, "RebootCount,TimeStamp,StateOfCharge,Voltage,ChargeCurrent,Temp \r\n"));
  DEBUG((EFI_D_WARN, "QcomChargerApp::%a TimeStamp,StateOfCharge,Voltage,ChargeCurrent,Temp \r\n",__FUNCTION__));
  // 3. 配置超时时间，如果配置了充电LED灯，超时时间为500ms，否则为3s
  /* Calculate Time to know if IDLE wait have reached to print logs and get next action */
  if(ChargerActionInfo.LedConfigType == EFI_QCOM_CHARGER_CHARGING_LED_TOGGLE){
    /* wait for IDLE duration to get status and print status info */
    Timeoutms = QCOM_CHARGER_TOGGLE_LED_WAIT_DURATION_IN_MS;		// 500ms
  }else{
    /* Normal wait for 3s*/
    Timeoutms      = QCOM_CHARGER_IDLE_WAIT_DURATION_IN_MS;			// 3s
  }

  do{
  	// 4. 循环获取当前的充电状态
    /* Get charging action to be taken */
    Status = pQcomChargerProtocol->GetChargingAction(&ChargingAction, &ChargerActionInfo);
    CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a ChargingAction = %d Status = %r LedConfigType = %d \r\n", __FUNCTION__, ChargingAction, Status, ChargerActionInfo.LedConfigType));
    // 5. 根据当前状态判断是否要开机、关机、继续充电
    switch(ChargingAction)
    {
      case EFI_QCOM_CHARGER_ACTION_GOOD_TO_BOOT:
      /*TBD Handle Charging actions with Switch statement */
      {
        Status = pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
        QcomChargerAppEvent_ExitLPM();
        QcomChargerAppEvent_DisplayTimerEvent(FALSE, FALSE);
        ExitChargingLoop = TRUE;
      }
      break;

      case EFI_QCOM_CHARGER_ACTION_SHUTDOWN:
        {
          /*Exit LPM and Display image before shutting down*/
          QcomChargerAppEvent_ExitLPM();
          QcomChargerAppEvent_DisplayTimerEvent(FALSE, FALSE);
        }
        Status = pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
        /* Control does not return here as in case to shutdown action */
        ExitChargingLoop = TRUE;
      break;

      case EFI_QCOM_CHARGER_ACTION_NO_CHARGE_WAIT:
        {
          /* Take Action to make sure charging is disabled */
          Status = pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
          CHARGERAPP_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a No Charge Wait ChrgAct = %d wait %d ms \r\n", __FUNCTION__, ChargingAction, QCOM_CHARGER_IDLE_WAIT_DURATION_IN_MS));
          WaitForTimeout (QCOM_CHARGER_IDLE_WAIT_DURATION_IN_MS, QCOM_CHARGER_TIMEOUT_WAIT_FOR_KEY, &UserKey);
          /*Check if WaitForTimeout returned due to key press */
          if((UserKey.UnicodeChar != 0x00) || (UserKey.ScanCode != 0x00))
          {
            DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Keypress detected \r\n", __FUNCTION__));
            QcomChargerAppEvent_KeyPressHandler(QCOMCHARGERAPP_EVENT_ANIMATION__LOW_BATTERY_NO_ANIM);
          }
        }
      break;
      case EFI_QCOM_CHARGER_ACTION_CRITICAL:
        {
          /*Exit LPM and Display image before shutting down*/
          QcomChargerAppEvent_ExitLPM(  );
          Status = pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
          /* Control does not return here as in case to shutdown action */
          ExitChargingLoop = TRUE;
          }
      break;
      case EFI_QCOM_CHARGER_ACTION_SHUTDOWN_USB_DC_PON_DISABLED:
          {
            /*Exit LPM and Display image before shutting down*/
            QcomChargerAppEvent_ExitLPM();
            CHARGERAPP_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Shutdown with PON disabled \r\n", __FUNCTION__));
            Status = pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
            /* Control does not return here as in case to shutdown action */
            ExitChargingLoop = TRUE;
        }
      break;
      case EFI_QCOM_CHARGER_ACTION_CONTINUE:
      default:
        /* Take Action */
        Status |= pQcomChargerProtocol->TakeAction(ChargingAction, &ChargerActionInfo);
        QcomChargerApp_LogParam(&ChargerActionInfo);
        CHARGERAPP_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Waiting for %d ms \r\n", __FUNCTION__, Timeoutms));
        /* Now wait for set Timeout */
        WaitForTimeout (Timeoutms, QCOM_CHARGER_TIMEOUT_WAIT_FOR_KEY, &UserKey);
        /*Check if WaitForTimeout returned due to key press */
        if((UserKey.UnicodeChar != 0x00) || (UserKey.ScanCode != 0x00))
        {
          DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Keypress detected \r\n", __FUNCTION__));
          QcomChargerAppEvent_KeyPressHandler(QCOMCHARGERAPP_EVENT_ANIMATION__CHARGING_ANIM);
        }
      break;
    }
  }while (FALSE == ExitChargingLoop);
  // 6. 开启10分钟的看门狗定时器
  /* Start 10min watchdog timer */
  gBS->SetWatchdogTimer(10 * 60, 0, 0, NULL);

  CHARGERAPP_DEBUG((EFI_D_WARN, "QcomChargerApp:: %a Exiting ChargingAction = %d \r\n", __FUNCTION__, ChargingAction));

  return Status;
}
```

# 充电驱动各函数分析

通过前面代码`gBS->LocateProtocol( &gQcomChargerProtocolGuid, NULL, (VOID **)&pQcomChargerProtocol )`
充电驱动是和`gQcomChargerProtocolGuid`绑定在一起，接下来，我们看下它的代码。

* `QcomPkg\SocPkg\AgattiPkg\LAA\Core.fdf`:
```
  #
  # Charger Driver
  #
  INF QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf

```

* `QcomPkg/SocPkg/AgattiPkg/LAA/ImageFv.fdf.inc`图片资源定义:
```C++
  FILE FREEFORM = 3E5584ED-05D4-4267-9048-0D47F76F4248 {
    SECTION UI = "battery_symbol_Soc10.bmp"
    SECTION RAW = QcomPkg/Application/QcomChargerApp/battery_symbol_Soc10.bmp
  }

  FILE FREEFORM = 4753E815-DDD8-402d-BF69-9B8C4EB7573E {
      SECTION UI = "battery_symbol_NoBattery.bmp"
      SECTION RAW = QcomPkg/Application/QcomChargerApp/battery_symbol_NoBattery.bmp
  }

  FILE FREEFORM = 03DED53E-BECD-428f-9F79-5ABA64C58445 {
      SECTION UI = "battery_symbol_Nocharger.bmp"
      SECTION RAW = QcomPkg/Application/QcomChargerApp/battery_symbol_Nocharger.bmp
  }

  FILE FREEFORM = 8b86cd38-c772-4fcf-85aa-345b2b3c1ab4 {
      SECTION UI = "battery_symbol_LowBatteryCharging.bmp"
      SECTION RAW = QcomPkg/Application/QcomChargerApp/battery_symbol_LowBatteryCharging.bmp
  }

  FILE FREEFORM = 3FD97907-93F1-4349-AF3C-3B68B0A5E626 {
      SECTION UI = "battery_symbol_LowBattery.bmp"
      SECTION RAW = QcomPkg/Application/QcomChargerApp/battery_symbol_LowBattery.bmp
  }
```

## 入口函数：QcomChargerInitialize()

* `BOOT.XF.1.4\boot_images\QcomPkg\Drivers\QcomChargerDxe\QcomCharger.c`:
```C++
EFI_STATUS QcomChargerInitialize(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable){
  /* Required Initialization */
  Status = ChargerPlatform_Init();
  Status = gBS->InstallMultipleProtocolInterfaces( &ImageHandle,
                      &gQcomChargerProtocolGuid, &QcomChargerProtocolImplementation, NULL );
  return Status;
}
```
可以看出，在入口函数中，只做了两件事，第一个为充电初化，第二就是将QcomChargerProtocolImplementation结构体函数指针与gQcomChargerProtocolGuid 绑定在一起，我们来看下充电初始化函数中做了啥。

1. 获取平台类型，如： EFI_PLATFORMINFO_TYPE_MTP
2. 加载充电配置文件 QcomChargerCfg.cfg，解析的内容保存在QCOM_CHARGER_PLATFORM_CFGDATA_TYPE gChargerPlatformCfgData 结构体中
3. 根据gChargerPlatformCfgData 中的配置信息，初始化充电相关的lib 库
4. 获取充电配置文件中的 BootToHLOSThresholdInMv = 3600, OsStandardBootSocThreshold = 7
5. 使能充电硬件 JEITA
6. 根据配置文件决定是否开启看门狗

* `QcomPkg\Drivers\QcomChargerDxe\QcomChargerPlatform.c`:
```C++
/* ChargerPlatform_Init(): This function locates and initializes ADC Protocal, Charger Protocal and other Protocols 
that are needed for that specific platform. It also loads the cfg file and initialize charger and FG accordingly. */
EFI_STATUS ChargerPlatform_Init( VOID )
{
  // 1. 获取平台类型，如： EFI_PLATFORMINFO_TYPE_MTP
  Status |= GetPlatformType(&PlatformType);

  // 2. 加载充电配置文件 QcomChargerCfg.cfg，解析的内容保存在gChargerPlatformCfgData 结构体中  /* Load CFG file */
  Status |= ChargerPlatformFile_ReadDefaultCfgData();

  // 3. 根据gChargerPlatformCfgData 中的配置信息，初始化充电相关的lib 库 /* Init Charger lib configs */
  Status |= ChargerLibCommon_InitConfiguration((chargerlib_cfgdata_type*)&gChargerPlatformCfgData);
  Status = ChargerPlatformFile_FileLogInit(gChargerPlatformCfgData);
  /* Init Charger lib */
  Status |= ChargerLibCommon_Init();

  // 4. 获取充电配置文件中的 BootToHLOSThresholdInMv = 3600, OsStandardBootSocThreshold = 7
  gThresholdVbatt = gChargerPlatformCfgData.BootToHLOSThresholdInMv;	
  gThresholdSoc   = gChargerPlatformCfgData.BootToHLOSThresholdInSOC;
 
  // 5. 使能充电硬件 JEITA /* Enable HW JEITA*/
  Status |= ChargerLib_EnableHWJeita(TRUE);

  // 6. 根据配置文件决定是否开启看门狗 /*Disable Charger wdog before applying config based wdog setting*/
  Status = ChargerLib_EnableWdog(FALSE);
  if((QCOM_CHG_WDOG_DISABLE_ON_EXIT == gChargerPlatformCfgData.EnableChargerWdog)|| (QCOM_CHG_WDOG_LEAVE_ENABLED_ON_EXIT == gChargerPlatformCfgData.EnableChargerWdog))
  {
    /* Enable Charger Watchdog*/
    Status = ChargerLib_EnableWdog(TRUE);
  }
  return Status;
}
```

充电配置文件实际路径为`QcomPkg/SocPkg/AgattiPkg/Settings/Charger/QcomChargerConfig_VbattTh.cfg`，在该文件中定义了一系列的充电参数。
```C++
//QcomPkg\SocPkg\AgattiPkg\LAA\Core.fdf:
  FILE FREEFORM = 45FE4B7C-150C-45da-A021-4BEB2048EC6F {
    SECTION UI = "QcomChargerCfg.cfg"
    SECTION RAW = QcomPkg/SocPkg/AgattiPkg/Settings/Charger/QcomChargerConfig_VbattTh.cfg
  }

//充电电流配置为200mA  	Charging termination current in milliamps
ChargingTermCurrent = 200

//电流ID电压误差容量8%  	Battery ID Tolerance Percentage 8%
BatteryIdTolerance = 8

//开机电压3.45v  Boot device to HLOS in case of unsupported battery or battery emulator. In millivolt*/
BootToHLOSThresholdInMv = 3450

//低电压3.2v  	Lowest Voltage at which device should shutdown gracefully value in mV
EmergencyShutdownVbatt = 3200

//是否加载电池配置文件 	Load Fuel Gauge Battery Profile profile for SOC estimation and accuracy
LoadBatteryProfile   = FALSE
```

## 充电状态 pQcomChargerProtocol->TakeAction()

在函数EFI_QcomChargerTakeAction() 中调用的是ChargerPlatform_TakeAction() 函数，我们来分析下：
电池状态的处理情况，分类如下几种：

1. 直接关机：QCOM_CHARGER_PLATFORM_ACTION_CRITICAL、QCOM_CHARGER_PLATFORM_ACTION_SHUTDOWN
2. 开始/停止 充电：QCOM_CHARGER_PLATFORM_ACTION_START_CHARGING、QCOM_CHARGER_PLATFORM_ACTION_STOP_CHARGING
3. 继续充电： QCOM_CHARGER_PLATFORM_ACTION_CONTINUE
4. 加载充电曲线配置文件：QCOM_CHARGER_PLATFORM_ACTION_PROFILE_LOAD
```C++
EFI_STATUS ChargerPlatform_TakeAction(EFI_QCOM_CHARGER_ACTION_TYPE ChargingAction, CONST QCOM_CHARGER_PLATFORM_ACTION_INFO *pChargerActionInfo)
{
  QCOM_CHARGER_BATT_STATUS_INFO CurrentBatteryStatus = {0};
  STATIC BOOLEAN bToggleLed = TRUE;
  CHARGERLIB_CHARGER_INPUT_STATUS ChargerInputStatus = {0};
  CHARGERLIB_ATTACHED_CHGR_TYPE     AttachedCharger   = CHARGERLIB_ATTACHED_CHGR__NONE;
  EFI_STATUS Status = EFI_SUCCESS;

  if(!pChargerActionInfo)
  {
    CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Invalid parameter \r\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  CurrentBatteryStatus = (QCOM_CHARGER_BATT_STATUS_INFO)pChargerActionInfo->BattStsInfo;

  switch (ChargingAction)
  {
  case EFI_QCOM_CHARGER_ACTION_CRITICAL:
         //print out and flush critical error messages 
         //Perform AFP
         CHARGER_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerDxe::%a Critical Error occurred. Shutting down \r\n", __FUNCTION__));
         ChargerLib_ForceSysShutdown(CHGAPP_RESET_AFP);
         break;

  case   EFI_QCOM_CHARGER_ACTION_SHUTDOWN:
         //print error message and trigger system shutdown
         //These errors will only be checked and handled when battery voltage is not high enough to boot and uefi charging is needed.
         CHARGER_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerDxe::%a Waiting for %d s \r\n", __FUNCTION__, QCOM_CHARGER_IDLE_WAIT_DURATION/QCOM_CHARGER_MS_TO_S));
         WaitForTimeout (QCOM_CHARGER_IDLE_WAIT_DURATION, TIMEOUT_WAIT_FOR_KEY, NULL);
         ChargerLib_ForceSysShutdown(CHGAPP_RESET_SHUTDOWN);
         break;

  case EFI_QCOM_CHARGER_ACTION_START_CHARGING:
          /* Assign entry voltage on warm boot and soc after profile is loaded or not loaded as in warm/cold boot */
          if(gChargerSharedInfo.uefi_entry_mV == QCOM_CHARGER_INVALID_VALUE_MARKER)
          {
            gChargerSharedInfo.uefi_entry_mV    = CurrentBatteryStatus.BatteryVoltage;
          }
          if (gChargerPlatformCfgData.ChargerLibCfgData.fg_cfg_data.LoadBatteryProfile == TRUE)
          {
            gChargerSharedInfo.uefi_entry_soc = CurrentBatteryStatus.StateOfCharge;
          }
          else
          {
            gChargerSharedInfo.uefi_entry_soc   = QCOM_CHARGER_INVALID_VALUE_MARKER;
          }
          CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Saving Entry VBatt  = %d SOC = %d \r\n", __FUNCTION__,
                          CurrentBatteryStatus.BatteryVoltage, CurrentBatteryStatus.StateOfCharge));

          Status = ChargerLib_GetChargingPath(&AttachedCharger);
          if ((EFI_SUCCESS == Status) && (CHARGERLIB_ATTACHED_CHGR__USB == AttachedCharger))
          {
            ChargerLib_InitializeCharging();
          }
         break;

  case EFI_QCOM_CHARGER_ACTION_STOP_CHARGING:
         if(TRUE == gChargingInitialized)
         {
           ChargerLib_ChargerEnable(FALSE);
           gChargingInitialized = FALSE;
         }
         if(gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config)
         {
           /* Turn Off Charging */
           bToggleLed = FALSE;
           ChargerLib_LedOn(bToggleLed);
         }
         break;

  case EFI_QCOM_CHARGER_ACTION_NO_CHARGE_WAIT:
  case EFI_QCOM_CHARGER_ACTION_TSENSE_HIGH_WAIT:
	    ChargerLib_HandleNoChargeAndWait();
         if(gChargerPlatformCfgData.ChargerLibCfgData.enable_charger_wdog)
         {
           /* Pet the watchdog if feature is enabled */
           ChargerLib_PetChgWdog();
         }
         break;
    
  case EFI_QCOM_CHARGER_ACTION_CONTINUE:
         if(gChargerPlatformCfgData.ChargerLibCfgData.enable_charger_wdog)
         {
           /* Pet the watchdog if feature is enabled */
           ChargerLib_PetChgWdog();
         }
         if(gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config)
         {
            /*  DEBUG ONLY */
           /* CHARGER_DEBUG((EFI_D_WARN, "QcomChargerDxe::%a SWLedToggleConfig = %d \r\n", __FUNCTION__, gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config)); */
           switch(gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config)
           {
             case QCOM_CHARGER_PLATFORM_CHARGING_LED_ON:
               bToggleLed = TRUE; /* Make sure to turn on flag as control can come back from wait state */
               ChargerLib_LedOn(bToggleLed);
               break;
             case QCOM_CHARGER_PLATFORM_CHARGING_LED_TOGGLE:
               ChargerLib_LedOn(bToggleLed);
               bToggleLed = (bToggleLed == TRUE)? FALSE: TRUE;
              break;
             case QCOM_CHARGER_PLATFORM_CHARGING_LED_OFF:
              /* will not reache here */
             default:
              break;
            }
         }
         else
         {
           /*  DEBUG ONLY */
           /* CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Charging Led Indication is off = %d \r\n", __FUNCTION__, gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config)); */
         }
         ChargerLib_GetChargerInputStatus(&ChargerInputStatus);
         CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a ICLFinal = %d mA, ICLMax = %d mA\r\n", __FUNCTION__,
                         ChargerInputStatus.ICLfinalMa, ChargerInputStatus.ICLMaxMa));
         /* Debug FG SRAM */
         ChargerLib_DumpSram(FALSE);
         break;

  case EFI_QCOM_CHARGER_ACTION_GOOD_TO_BOOT:
        /* Assign Exit voltage and soc */
        gChargerSharedInfo.uefi_exit_mV    = CurrentBatteryStatus.BatteryVoltage;
        if (gChargerPlatformCfgData.ChargerLibCfgData.fg_cfg_data.LoadBatteryProfile == TRUE)
        {
          gChargerSharedInfo.uefi_exit_soc = CurrentBatteryStatus.StateOfCharge;
        }
        else
        {
          gChargerSharedInfo.uefi_exit_soc   = QCOM_CHARGER_INVALID_VALUE_MARKER;
        }
        CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Saving Exit VBat = %d Soc = %d \r\n", __FUNCTION__,
                         CurrentBatteryStatus.BatteryVoltage, CurrentBatteryStatus.StateOfCharge));
        /* Save Smem Info ignoring return status as XBL loader changes are not ready yet*/
        ChargerPlatform_SaveChargeInfoToSmem(&gChargerSharedInfo);
		if (gChargerPlatformCfgData.DCInCfgData.DCINDisableOnExit)
        {
          //suspend DCIn 
          (void)ChargerLib_DcinSuspend(TRUE);
        }
        
        /* Turn Off Charging */
        bToggleLed = FALSE;
        ChargerLib_LedOn(bToggleLed);
        break;

  case EFI_QCOM_CHARGER_PLATFORM_ACTION_PROFILE_LOAD:
        /* Assign only entry voltage for smem before profile load, soc will be update after profile is loaded in next action which is start charging action*/
        gChargerSharedInfo.uefi_entry_mV    = CurrentBatteryStatus.BatteryVoltage;
        ChargerPlatform_LoadProfile(pChargerActionInfo);
        break;

  case EFI_QCOM_CHARGER_ACTION_SHUTDOWN_USB_DC_PON_DISABLED:
      //print error message and trigger system shutdown
      //These errors will only be checked and handled when battery voltage is not high enough to boot and uefi charging is needed.
      CHARGER_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerDxe::%a Waiting for %d s \r\n", __FUNCTION__, QCOM_CHARGER_IDLE_WAIT_DURATION/QCOM_CHARGER_MS_TO_S));
      WaitForTimeout (QCOM_CHARGER_IDLE_WAIT_DURATION, TIMEOUT_WAIT_FOR_KEY, NULL);
      ChargerLib_ForceSysShutdown(CHGAPP_RESET_SHUTDOWN_USB_DC_PON_DISABLED);
      break;

        break;
  case EFI_QCOM_CHARGER_ACTION_FACTORY_MODE_BOOT_TO_HLOS:
	  CHARGER_FILE_UART_DEBUG((EFI_D_WARN, "QcomChargerDxe::Factory Mode - Boot to HLOS\r\n"));
	  break;
    default:
        CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Action Passed = %d \r\n", __FUNCTION__,ChargingAction));
      break;
  }
  
  return EFI_SUCCESS;
}
```

## 获取当前充电状态 pQcomChargerProtocol->GetChargingAction()

1. 检测充电相关错误，如函数运行失败直接通关机。
2. 根据前面的错误进行不同的处理：电池不存在，调试主板，温度异常，电池电压异常，充电电源异常
3. 默认LED 灯为闪烁模式。
4. 加载电池曲线参数
5. 检测充电过程的所有错误，并进行相应的处理
6. 获取电池状态
7. 获取当前充电的输入方式，如 USB、DCIN
8. 当一切准备就绪时，配置 pActionType=QCOM_CHARGER_PLATFORM_ACTION_START_CHARGING
9. 如果状态不符，则获取当前的充电状态
10. 如果是不在充电，则dump打印寄存器信息，如果是无线充电，配置为NO_CHARGE_WAIT，否则配置为START_CHARGING开始充电，如果是正在充电，则配置为 CONTINUE
11. 如果充电线已经连接，则触发重新插入动作
12. 检查当前是否满足正常开机条件，如果满足的话，更新 pActionType 为 QCOM_CHARGER_PLATFORM_ACTION_GOOD_TO_BOOT
```C++
EFI_STATUS ChargerPlatform_GetChargingAction(EFI_QCOM_CHARGER_ACTION_TYPE *pActionType, QCOM_CHARGER_PLATFORM_ACTION_INFO *pChargerActionInfo, BOOLEAN vbattChecking)
{
  EFI_STATUS Status                = EFI_SUCCESS;
  BOOLEAN    bChargerReinserted    = FALSE, DAMConnectSts = FALSE;
  CHARGERLIB_CHARGING_ERROR_TYPES           ErrorType  = CHARGERLIB_CHARGING_ERROR_NONE;
  STATIC EFI_QCOM_CHARGER_ACTION_TYPE  PrevChargerAction = EFI_QCOM_CHARGER_ACTION_INVALID;
  BOOLEAN                                   ChargingEnabled    = FALSE;

  if (!pChargerActionInfo || !pActionType)
    return EFI_INVALID_PARAMETER;

  /* Check if factory cable is connected */
  if (gChargerPlatformCfgData.ChargerLibCfgData.schg_cfg_data.EnDebugAccessMode)
  {
	  Status |= ChargerLib_GetDAMConnectStatus(&DAMConnectSts);
	  if (EFI_SUCCESS != Status)
	  {
		  CHARGER_DEBUG((EFI_D_ERROR, "QcomChargerDxe:: %a Error Getting Debug Accessory Mode Status = %r.\r\n", __FUNCTION__, Status));
	  }
	  if (DAMConnectSts)
	  {
		  *pActionType = EFI_QCOM_CHARGER_ACTION_FACTORY_MODE_BOOT_TO_HLOS;
		  PrevChargerAction = *pActionType;
		  return Status;
	  }
  }

  /* Get Error like debug board or battery not detected first */
  Status |= ChargerLib_GetErrors(vbattChecking, &ErrorType);
  if(EFI_SUCCESS != Status)
  {
    CHARGER_DEBUG(( EFI_D_ERROR, "QcomChargerDxe:: %a Error Getting Battery Error = %r.\r\n", __FUNCTION__, Status));
    *pActionType = EFI_QCOM_CHARGER_ACTION_SHUTDOWN;
    PrevChargerAction = *pActionType;
    return Status;
  }

  if((CHARGERLIB_CHARGING_ERROR_BATTERY_NOT_DETECTED == ErrorType ) || (CHARGERLIB_CHARGING_ERROR_DEBUG_BOARD == ErrorType ) ||
    (CHARGERLIB_DEVICE_ERROR == ErrorType ) || (CHARGERLIB_CHARGING_ERROR_UNKNOWN_BATTERY == ErrorType ) ||
    (CHARGERLIB_CHARGING_ERROR_TSENSE_CRITICAL == ErrorType) || (CHARGERLIB_CHARGING_ERROR_TSENSE_TIMEOUT == ErrorType) || (CHARGERLIB_CHARGING_ERROR_TSENSE_HIGH == ErrorType))
  {
    Status = ChargerLib_GetErrorAction(ErrorType, (((CHARGERLIB_ERROR_ACTION_TYPE*)pActionType)));
    PrevChargerAction = *pActionType;
    CHARGER_DEBUG((EFI_D_WARN, "QcomChargerDxe:: %a pActionType = %d.\r\n", __FUNCTION__, *pActionType));
    /*If there is a battery error, return */
    return Status;
  }

  /* Assign Led config to toggle led */
  pChargerActionInfo->LedConfigType = (QCOM_CHARGER_PLATFORM_CHARGING_LED_CONFIG_TYPE)gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config;

  if (PrevChargerAction == EFI_QCOM_CHARGER_ACTION_INVALID)
  {
    /* Load profile if SocBasedboot is true or profile load is enabled */
    if((gChargerPlatformCfgData.ChargerLibCfgData.soc_based_boot== TRUE ) || (gChargerPlatformCfgData.ChargerLibCfgData.fg_cfg_data.LoadBatteryProfile == TRUE ))
    {
      Status = ChargerPlatform_ProfileLoadingInit(pActionType, pChargerActionInfo, ErrorType);
      PrevChargerAction = *pActionType;

      /* Return since action is decided for this Invalid charging action case */
      return Status;
    }
    else
    {
      CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Battery Profile Loading Not Required \r\n", __FUNCTION__));
    }
  }

  Status |= ChargerLib_GetBatteryStatus((chargerlib_batt_status_info*)&(pChargerActionInfo->BattStsInfo));
  if (EFI_SUCCESS != Status)
  {
    CHARGER_DEBUG((EFI_D_WARN, "QcomChargerDxe:: %a Error Getting Battery Status = %r.\r\n", __FUNCTION__, Status));
    *pActionType = EFI_QCOM_CHARGER_ACTION_STOP_CHARGING;
    PrevChargerAction = *pActionType;

    /*If there is an error, return since action is decided */
    return Status;
  }

  if(CHARGERLIB_CHARGING_ERROR_NONE != ErrorType)
  {
    Status = ChargerLib_GetErrorAction(ErrorType, (((CHARGERLIB_ERROR_ACTION_TYPE*)pActionType)));
    PrevChargerAction = *pActionType;

    /*If there is a battery error, return */
    return Status;
  }

  Status = ChargerLib_GetChargingPath(&pChargerActionInfo->ChargerAttached);
  if (EFI_SUCCESS != Status)
  {
    CHARGER_DEBUG((EFI_D_WARN, "QcomChargerDxe:: %a Error Getting Power Path = %r.\r\n", __FUNCTION__, Status));
    *pActionType = EFI_QCOM_CHARGER_ACTION_STOP_CHARGING;
    PrevChargerAction = *pActionType;
    return Status;
  }

  if (((QCOM_CHARGER_PLATFORM_CHARGER_ATTACHED_USB == pChargerActionInfo->ChargerAttached) || (QCOM_CHARGER_PLATFORM_CHARGER_ATTACHED_DCIN == pChargerActionInfo->ChargerAttached)) &&
    ((EFI_QCOM_CHARGER_ACTION_INVALID == PrevChargerAction) ||
    (EFI_QCOM_CHARGER_PLATFORM_ACTION_PROFILE_LOAD == PrevChargerAction) ||
    (EFI_QCOM_CHARGER_ACTION_NO_CHARGE_WAIT == PrevChargerAction)))
  {
    *pActionType = EFI_QCOM_CHARGER_ACTION_START_CHARGING;
  }
  else
  {
    Status = ChargerLib_GetChargingStatus(&ChargingEnabled);
    if(EFI_SUCCESS != Status)
    {
      CHARGER_DEBUG((EFI_D_WARN, "ChargerLib::%a Error Getting Charging Status = %r \r\n", __FUNCTION__, Status));
      return Status;
    }

    if(FALSE == ChargingEnabled)
    {
      /*Charger register dump in case need to determine why charging is disabled*/
		Status |= ChargerLib_DumpChargerPeripheral();     
    }
    else
    {
        /* Charging already started, go to continue. */
      *pActionType = EFI_QCOM_CHARGER_ACTION_CONTINUE;
        /* Assign Led config to toggle led */
        /* pChargerActionInfo->LedConfigType = (QCOM_CHARGER_PLATFORM_CHARGING_LED_CONFIG_TYPE)gChargerPlatformCfgData.ChargerLibCfgData.charger_led_config;*/
      }

    if (QCOM_CHARGER_PLATFORM_CHARGER_ATTACHED_USB == pChargerActionInfo->ChargerAttached)
    {
      /* Check if charger was swapped/re-inserted */
      Status = ChargerLib_WasChargerReinserted(&bChargerReinserted);
      if(EFI_SUCCESS != Status )
      {
        CHARGER_DEBUG((EFI_D_WARN, "ChargerLib::%a Error = %d Error Checking Charger Re-insertion. \r\n", __FUNCTION__, Status));
        return Status;
      }

      if(bChargerReinserted)
      {
        //Status = ChargerLib_CheckAPSDResults();
        Status = ChargerLib_ReRunAicl();
      }
    }
  }

  Status |= ChargerPlatform_CheckIfOkToBoot(pActionType, *pChargerActionInfo, pChargerActionInfo->BattStsInfo);

  CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Action Returned = %d \r\n", __FUNCTION__,*pActionType));

  PrevChargerAction = *pActionType;

  return Status;
}

EFI_STATUS ChargerPlatform_CheckIfOkToBoot
(
  EFI_QCOM_CHARGER_ACTION_TYPE *pActionType,
  QCOM_CHARGER_PLATFORM_ACTION_INFO  ChargerActionInfo, 
  QCOM_CHARGER_BATT_STATUS_INFO      CurrentBatteryStatus
)
{
  switch(gChargerPlatformCfgData.ChargerLibCfgData.soc_based_boot)
  {
    case FALSE:
      if(QCOM_CHARGER_PLATFORM_CHARGER_ATTACHED_DCIN == ChargerActionInfo.ChargerAttached)
      {
        if (CurrentBatteryStatus.BatteryVoltage >= gChargerPlatformCfgData.DCInCfgData.DCInBootThreshold)
        {
          *pActionType = EFI_QCOM_CHARGER_ACTION_GOOD_TO_BOOT;
          CHARGER_DEBUG((EFI_D_WARN, "QcomChargerDxe:: %a Good To Boot To HLOS BatteryVoltage= %d DCIn threshold = %d \r\n",
			  __FUNCTION__, CurrentBatteryStatus.BatteryVoltage, gChargerPlatformCfgData.DCInCfgData.DCInBootThreshold));
        }
      }
      else if (CurrentBatteryStatus.BatteryVoltage >= gThresholdVbatt) //重要，判断电池是否达到正常开机阈值
      {
        *pActionType = EFI_QCOM_CHARGER_ACTION_GOOD_TO_BOOT;
         CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Good To Boot To HLOS BatteryVoltage= %d gThresholdVbatt = %d \r\n",
                        __FUNCTION__, CurrentBatteryStatus.BatteryVoltage, gThresholdVbatt));
      }
      else
      { /* TBD comment out */
        CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a NOT Enough BatteryVoltage To Boot = %d gThresholdVbatt = %d \r\n",
                        __FUNCTION__, CurrentBatteryStatus.BatteryVoltage, gThresholdVbatt));
      }
      break;

    case TRUE:
      if(CurrentBatteryStatus.StateOfCharge >= gThresholdSoc)
      {
        *pActionType = EFI_QCOM_CHARGER_ACTION_GOOD_TO_BOOT;
        CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Good To Boot To HLOS StateOfCharge= %d gThresholdSoc = %d \r\n",
                       __FUNCTION__, CurrentBatteryStatus.StateOfCharge, gThresholdSoc));
      }
      else
      { /* TBD comment out */
        CHARGER_DEBUG(( EFI_D_WARN, "QcomChargerDxe:: %a Not Enough StateOfCharge= %d To Boot gThresholdSoc = %d \r\n",
                       __FUNCTION__, CurrentBatteryStatus.StateOfCharge, gThresholdSoc));
      }
    break;
    default:
    break;
  }
  
  return EFI_SUCCESS;
}
```