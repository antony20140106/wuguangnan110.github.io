# Android常用开发指令

记录一下开发Android时常用的开发指令


# 查找某个app安装位置

```
1|M92xx:/cache $ pm -l|grep market
package:com.xxx.market.android.app
M92xx:/cache $ pm -p com.xxx.market.android.app
package:/data/app/~~dE4wALCxRS61DxReGFCuGA==/com.xxx.market.android.app-iMGWfPBNEhmQVKssxH1F0A==/base.apk
```

# adb 广播

例如，我们要发送一条名为com.example.MY_ACTION的广播，命令如下：
```
adb shell am broadcast -a com.example.MY_ACTION
```

当然，在发出这条广播时，如果需要设置一些Extra参数，也可以通过在命令中添加<-e key value>的形式实现，示例如下：
向BroadcastReceiver中发送一个名为com.example.MY_ACTION的广播，并且传参 extras_key=extras_value
```
adb shell am broadcast -a com.example.MY_ACTION -n com.example/.BroadcastReceiver --es extras_key extras_value
```