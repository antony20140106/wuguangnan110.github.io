# 概述

电池一般工作温度在-10~60度之间，超过该范围会对电池寿命有影响，需要做关机处理，BatteryServers里面有对高温关机进行处理，不过是68度，我们需要自己新增。

# BatteryServers高温关机

BatteryServers做了高温关机，默认是68度高温关机，我们目前不用这个。
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

# bms高温关机

目前BMS要求只要低温、高温、低压关机报警功能，首先看看接收异常广播`xxx.intent.action.BATTERY_ABNORMAL`入口，主要完成以下事情：
  * 1. 接收`xxx.intent.action.BATTERY_ABNORMAL`异常广播，上传的type都是向左移位的，所以需要将原始的tpye根据公式`Math.log`求2的对数的解析出来。
  * 2. 将type值用intent传到`BatteryWarningToShutdown`的activity中进行处理。
* `QSSI.12/packages/apps/BatteryWarning/src/com/xxx/batterywarning/BatteryWarningReceiver.java`:
```java
public class BatteryWarningReceiver extends BroadcastReceiver {
     // private static final String ACTION_IPO_BOOT = "android.intent.action.ACTION_BOOT_IPO";
    private static final String ACTION_BATTERY_WARNING = "xxx.intent.action.BATTERY_ABNORMAL";
    private static final String ACTION_BATTERY_NORMAL = "xxx.intent.action.BATTERY_NORMAL";
    private static final String TAG = "BatteryWarningReceiver";
    private static final String SHARED_PREFERENCES_NAME = "battery_warning_settings";
    private Context mContext;

    // Thermal over temperature
    private static final String ACTION_THERMAL_WARNING = "xxx.intent.action.THERMAL_DIAG";

   public void onReceive(Context context, Intent intent) {
        mContext = context;
        String action = intent.getAction();
        Log.d(TAG, "action = " + action);
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Log.d(TAG, action + " clear battery_warning_settings shared preference");
            SharedPreferences.Editor editor = getSharedPreferences().edit();
            editor.clear();
            editor.apply();
        } else if (ACTION_BATTERY_WARNING.equals(action)) {
            Log.d(TAG, action + " start activity according to shared preference");
            int type = intent.getIntExtra("type", -1);
            Log.d(TAG, "type = " + type);
            type = (int) (Math.log(type) / Math.log(2));
            Log.d(TAG, "type = " + type);
            if(type == BatteryWarningToShutdown.xxx_BAT_NC_UV | 
			   type == BatteryWarningToShutdown.xxx_BAT_LOW_TEMP | 
			   type == BatteryWarningToShutdown.xxx_BAT_HIGH_TEMP){
                ShowBatteryWaringToShutdownDialog(type);
            }
        }
    }

    private void ShowBatteryWaringToShutdownDialog(int type){
        Intent shutDownIntent = new Intent();
        shutDownIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_CLEAR_TOP
                | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        shutDownIntent.setClass(mContext, BatteryWarningToShutdown.class);
        shutDownIntent.putExtra(BatteryWarningToShutdown.KEY_TYPE, type);
        mContext.startActivity(shutDownIntent);
    }

}
```

* 处理异常如下：
  * 1. `onCreate`接收到type值，调用弹框`showVoltageUnderWarningDialog`，这里启动10s定时器，并监听`BatteryManager.EXTRA_PLUGGED`广播，拔出usb则关闭定时器。
  * 2. 定时器中发送给handleMessage刷新弹框倒计时时间，mShutDownTime--为0时则关机。
* `QSSI.12/packages/apps/BatteryWarning/src/com/xxx/batterywarning/BatteryWarningToShutdown.java`:
```java
public class BatteryWarningToShutdown extends Activity {
    private static final String TAG = "BatteryWarningToShutdown";
    protected static final String KEY_TYPE = "type";
    private static final Uri WARNING_SOUND_URI = Uri.parse("file:///system/media/audio/ui/VideoRecord.ogg");

    protected static final int xxx_BAT_NC_UV = 18;  //under voltage,power off
    protected static final int xxx_BAT_LOW_TEMP = 5;  //under temp,power off
	 protected static final int xxx_BAT_HIGH_TEMP = 1;  //over temp,power off

    private Handler mHandler  = new Handler(){
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if(msg.what == 1){
                mShutDownTime--;
                if(mShutDownTime == 0){
                    mContent.setText(getString(R.string.xxx_shutdown_now_text));
                    mTimer.cancel();
                    stopRingtone();
                    try{
                        Log.d(TAG, "xxx under voltage will shutdown");
                        IPowerManager pm = IPowerManager.Stub.asInterface(
                            ServiceManager.getService(Context.POWER_SERVICE));
                        pm.shutdown(false, "under voltage", false);
                    }catch(RemoteException e){
                        Log.d(TAG, "xxx battery under voltage shutdown fail.");
                        e.printStackTrace();
                    }
                } else {
					if (mType == xxx_BAT_LOW_TEMP) {
						mContent.setText(getString(R.string.xxx_low_temp_shutdown_time_text, mShutDownTime));
					}
					else {
						mContent.setText(getString(R.string.xxx_high_temp_shutdown_time_text, mShutDownTime));
					}
                }
            }
        }
    };

    private Timer mTimer = new Timer(true);

    private TimerTask mTask = new TimerTask() {
        public void run() {
            Message msg = new Message();
            msg.what = 1;
            mHandler.sendMessage(msg);
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_BATTERY_CHANGED.equals(action)) {
                boolean plugged = (0 != intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0));
                if (plugged) {
                    Log.d(TAG, "receive ACTION_BATTERY_CHANGED broadcast plugged:" + plugged + ", finish");
                    mTimer.cancel();
                    stopRingtone();
                    finish();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        mType = intent.getIntExtra(KEY_TYPE, -1);
        Log.d(TAG, "onCreate, mType is " + mType);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.xxx_battery_notify_warning);

        mTitleView = (TextView) findViewById(R.id.title);
        mContent = (TextView) findViewById(R.id.content);
        mYesButton = (Button) findViewById(R.id.yes);
        if(mType == xxx_BAT_NC_UV){
            showVoltageUnderWarningDialog();
        } if(mType == xxx_BAT_LOW_TEMP){
            showTempLowWarningDialog();
        } else if(mType == xxx_BAT_HIGH_TEMP){
			showTempHighWarningDialog();
		}
		else {
            finish();
        }
    }

    private void showVoltageUnderWarningDialog() {
        mShutDownTime = 10;
        mTitleView.setText(R.string.xxx_battery_voltage_under);
        mContent.setText(getString(R.string.xxx_shutdown_time_text, mShutDownTime));

        mYesButton.setText(getString(android.R.string.yes));
        mYesButton.setOnClickListener(mYesListener);

        registerReceiver(mReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        playAlertSound(WARNING_SOUND_URI);
        mTimer.schedule(mTask, 0, 1*1000);
    }
}
```