# 概述

电池一般工作温度在-10~60度之间，超过该范围会对电池寿命有影响，需要做关机处理，BatteryServers里面有对高温关机进行处理，不过是68度，我们需要自己新增。

# BatteryServers高温关机

* `frameworks/base/services/core/java/com/android/server/BatteryService.java`:
```java
public BatteryService(Context context) {
       mCriticalBatteryLevel = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_criticalBatteryWarningLevel);
        mLowBatteryWarningLevel = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_lowBatteryWarningLevel);
        mLowBatteryCloseWarningLevel = mLowBatteryWarningLevel + mContext.getResources().getInteger(
                com.android.internal.R.integer.config_lowBatteryCloseWarningBump);
        mShutdownBatteryTemperature = mContext.getResources().getInteger(
                com.android.internal.R.integer.config_shutdownBatteryTemperature);

    private void shutdownIfOverTempLocked() {
        // shut down gracefully if temperature is too high (> 68.0C by default)
        // wait until the system has booted before attempting to display the
        // shutdown dialog.
        if (mHealthInfo.batteryTemperature > mShutdownBatteryTemperature) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mActivityManagerInternal.isSystemReady()) {
                        Intent intent = new Intent(Intent.ACTION_REQUEST_SHUTDOWN);
                        intent.putExtra(Intent.EXTRA_KEY_CONFIRM, false);
                        intent.putExtra(Intent.EXTRA_REASON,
                                PowerManager.SHUTDOWN_BATTERY_THERMAL_STATE);
                        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        mContext.startActivityAsUser(intent, UserHandle.CURRENT);
                    }
                }
            });
        }
    }

}
```

* `frameworks/base/core/res/res/values/config.xml`:
```xml
    <!-- Display low battery warning when battery level dips to this value.
         Also, the battery stats are flushed to disk when we hit this level.  -->
    <integer name="config_criticalBatteryWarningLevel">5</integer>

    <!-- Shutdown if the battery temperature exceeds (this value * 0.1) Celsius. -->
    <integer name="config_shutdownBatteryTemperature">680</integer> //高温关机温度

    <!-- Display low battery warning when battery level dips to this value -->
    <integer name="config_lowBatteryWarningLevel">15</integer> //低电提醒

    <!-- The default suggested battery % at which we enable battery saver automatically.  -->
    <integer name="config_lowBatteryAutoTriggerDefaultLevel">15</integer> //15%电量自动进入省电模式

    <!-- The app which will handle routine based automatic battery saver, if empty the UI for
         routine based battery saver will be hidden -->
    <string name="config_batterySaverScheduleProvider"></string>

    <!-- Close low battery warning when battery level reaches the lowBatteryWarningLevel
         plus this -->
    <integer name="config_lowBatteryCloseWarningBump">5</integer> //关闭低电量模式警告电量
```